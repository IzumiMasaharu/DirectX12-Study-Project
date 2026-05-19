#include "D3D12RenderHost.h"

using namespace Microsoft::WRL;

D3D12RenderHost* D3D12RenderHost::appPtr = nullptr;

D3D12RenderHost::D3D12RenderHost(HINSTANCE hInstance) : windowsManager(hInstance,this), appInstance(hInstance)
{
    appPtr = this;
}
D3D12RenderHost::~D3D12RenderHost()
{
    if (d3dDevice != nullptr)
        FlushCommandQueue();
    if (appPtr == this)
        appPtr = nullptr;
}

// 派生类需覆写此虚函数以编写自己需要的窗口消息处理方式
LRESULT D3D12RenderHost::wndMsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// 应用运行
int D3D12RenderHost::RunMessagePump() const
{
    MSG msg = { nullptr };

    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}

void D3D12RenderHost::ControlLoop()
{
	while (isAppRunning)
	{
		if (!isAppPaused)
		{
			gameTimer.Tick();
			Update(gameTimer);

			// 通知渲染线程可以渲染
			{
				std::unique_lock<std::mutex> lock(renderMutex);
				isFrameReady = true;
				isFrameRendered = false;
				renderCV.notify_one();
			}

			// 等待渲染线程完成本帧
			while (!isFrameRendered && isRenderThreadRunning)
			{
				std::this_thread::yield();
			}
		}
	}
}

// 初始化DirectX 3D
bool D3D12RenderHost::InitializeD3D12Device()
{
// 启动D3D调试层
#if defined(DEBUG)||defined(_DEBUG)
    {
        ComPtr<ID3D12Debug> debugCotroller;
        ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugCotroller)));
        debugCotroller->EnableDebugLayer();
    }
#endif
    // 创建DXGI Factory
    ThrowIfFailed(CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory)));
    // 创建硬件D3D设备
    HRESULT hardwareResulte = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3dDevice));
    // 若创建失败，回退至WARP设备
    if (FAILED(hardwareResulte))
    {
        ComPtr<IDXGIAdapter> pWARPAdapter;
        ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWARPAdapter)));
        ThrowIfFailed(D3D12CreateDevice(pWARPAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3dDevice)));
    }
    // 创建围栏
    ThrowIfFailed(d3dDevice->CreateFence(currentFenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
    
    // 检测MSAA级别支持
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS mQualityLevel;
    mQualityLevel.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    mQualityLevel.Format = backBufferFormat;
    mQualityLevel.NumQualityLevels = 0;
    mQualityLevel.SampleCount = 4;
    ThrowIfFailed(d3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &mQualityLevel, sizeof(mQualityLevel)));
    MSAA4xQualityLevel = mQualityLevel.NumQualityLevels;

    // 获取描述符大小
    rtvDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    dsvDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    cbs_srv_uavDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

#ifdef _DEBUG
    LogAdapters();
#endif

    CreateCommandObjects();
    CreateSwapChain();
    CreateSwapChainDescriptorHeaps();

    return true;
}

// 创建命令队列、命令分配器、命令列表
void D3D12RenderHost::CreateCommandObjects()
{
    // 填写描述命令队列的结构体
    D3D12_COMMAND_QUEUE_DESC qd = {};
    qd.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    qd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    // 创建
    ThrowIfFailed(d3dDevice->CreateCommandQueue(&qd, IID_PPV_ARGS(&commandQueue)));
    ThrowIfFailed(d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS( commandAllocator.GetAddressOf())));
    ThrowIfFailed(d3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(commandList.GetAddressOf())));

    commandList->Close();
}

// 创建交换链
void D3D12RenderHost::CreateSwapChain()
{
    // 释放之前所创建的的交换链，重新创建
    swapChain.Reset();
    // 填写描述所创建交换链的结构体
    DXGI_SWAP_CHAIN_DESC scd;
    scd.BufferDesc.Width = clientWidth;
    scd.BufferDesc.Height = clientHeight;
    scd.BufferDesc.Format = backBufferFormat; // 显示格式
    scd.BufferDesc.RefreshRate.Numerator = 120; // 最高刷新率
    scd.BufferDesc.RefreshRate.Denominator = 1; // 最低刷新率
    scd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED; // 是否缩放
    scd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED; // 逐行扫描还是隔行扫描
    scd.SampleDesc.Count = 1; // 多重采样采样数量
    scd.SampleDesc.Quality = 0; // 多重采样质量级别
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.BufferCount = SwapChainBufferCount;
    scd.Windowed = true; // 窗口显示还是全屏显示
    scd.OutputWindow = mainWndHwnd; // 图像输出窗口句柄
    scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    ThrowIfFailed(dxgiFactory->CreateSwapChain(commandQueue.Get(), &scd, swapChain.GetAddressOf()));
}

// 创建描述符堆(RTV和DSV)
void D3D12RenderHost::CreateSwapChainDescriptorHeaps()
{
    // 创建RTV描述符堆
    D3D12_DESCRIPTOR_HEAP_DESC RTVHeapDesc ;
    RTVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    RTVHeapDesc.NodeMask = 0;
    RTVHeapDesc.NumDescriptors = SwapChainBufferCount;
    RTVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&RTVHeapDesc, IID_PPV_ARGS(rtvHeap.GetAddressOf())));
    // 创建DSV描述符堆
    D3D12_DESCRIPTOR_HEAP_DESC DSVHeapDesc ;
    DSVHeapDesc.NumDescriptors = 1;
    DSVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    DSVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    DSVHeapDesc.NodeMask = 0;
    ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&DSVHeapDesc, IID_PPV_ARGS(dsvHeap.GetAddressOf())));
}

// 加载枚举所有显示适配器
void D3D12RenderHost::LogAdapters()
{
    IDXGIAdapter* Adapter = nullptr;
    std::vector<IDXGIAdapter*> AdapterList;
    for (UINT i = 0; dxgiFactory->EnumAdapters(i, &Adapter) != DXGI_ERROR_NOT_FOUND; i++)
    {
        DXGI_ADAPTER_DESC ad;
        Adapter->GetDesc(&ad);

        std::wstring AdapterText = L"可用显示适配器 " + std::to_wstring(i + 1) + L":";
        AdapterText += ad.Description;
        AdapterText += L"\n";
        OutputDebugString(AdapterText.c_str());

        AdapterList.push_back(Adapter);
    }

    for (auto ad : AdapterList)
    {
        LogAdapterOutputs(ad);

        ad->Release();
        ad = nullptr;
    }
}

// 加载枚举所有显示输出
void D3D12RenderHost::LogAdapterOutputs(IDXGIAdapter* adapter)
{
    IDXGIOutput* Output = nullptr;
    for (UINT i = 0; adapter->EnumOutputs(i, &Output) != DXGI_ERROR_NOT_FOUND; i++)
    {
        DXGI_OUTPUT_DESC od;
        Output->GetDesc(&od);

        std::wstring OutputText = L"可用显示输出 " + std::to_wstring(i + 1) + L":";
        OutputText += od.DeviceName;
        OutputText += L"\n";
        OutputDebugString(OutputText.c_str());

        LogAdapterDisplayModes(Output, DXGI_FORMAT_B8G8R8A8_UNORM);

        ReleaseCom(Output);
    }
}

// 加载枚举所有显示输出格式
void D3D12RenderHost::LogAdapterDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
{
    UINT count = 0;
    UINT flags = 0;

    output->GetDisplayModeList(format, flags, &count, nullptr); // 将参数pDesc设为nullptr可获取满足条件的显示模式的数量，并存入count中
    std::vector<DXGI_MODE_DESC> modelist(count);
    output->GetDisplayModeList(format, flags, &count, &modelist[0]);

    uint32_t x = 0;
    for (auto& i : modelist)
    {
        x++;
        UINT nu = i.RefreshRate.Numerator;
        UINT de = i.RefreshRate.Denominator;
        std::wstring DisplayModeText =
            L"显示模式" + std::to_wstring(x) +
            L"宽度：" + std::to_wstring(i.Width) + L"  " +
            L"高度：" + std::to_wstring(i.Height) + L"  " +
            L"刷新率：" + std::to_wstring(nu) + L"~" + std::to_wstring(de) +
            L"\n";

        OutputDebugString(DisplayModeText.c_str());
    }
}

// 刷新命令队列
void D3D12RenderHost::FlushCommandQueue()
{
    // 围栏法
    currentFenceValue++;
    ThrowIfFailed(commandQueue->Signal(fence.Get(), currentFenceValue));
    if (fence->GetCompletedValue() < currentFenceValue)
    {
        HANDLE EventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);

        ThrowIfFailed(fence->SetEventOnCompletion(currentFenceValue, EventHandle));

        if (EventHandle)
        {
            WaitForSingleObject(EventHandle, INFINITE);
            CloseHandle(EventHandle);
        }
    }
}

// 获取指向当前缓冲区的指针
ID3D12Resource* D3D12RenderHost::CurrentBackBuffer()const
{
    return swapChainBuffer[currentBackBuffer].Get();
}
// 获取当前后台缓冲区的RTV
D3D12_CPU_DESCRIPTOR_HANDLE D3D12RenderHost::CurrentBackBufferView()const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
        rtvHeap->GetCPUDescriptorHandleForHeapStart(),
        currentBackBuffer, rtvDescriptorSize);
}
// 获取当前后台缓冲区的DSV
D3D12_CPU_DESCRIPTOR_HANDLE D3D12RenderHost::DepthStencilBufferView()const
{
    return dsvHeap->GetCPUDescriptorHandleForHeapStart();
}

// 重新设置DX相关尺寸属性
void D3D12RenderHost::Resize()
{
    assert(d3dDevice);
    assert(commandAllocator);
    assert(swapChain);

    FlushCommandQueue();

    commandList->Reset(commandAllocator.Get(), nullptr);

    for (UINT i = 0; i < SwapChainBufferCount; i++)
        swapChainBuffer[i].Reset();
    depthStencilBuffer.Reset();

    swapChain->ResizeBuffers(
        SwapChainBufferCount,
        clientWidth, clientHeight,
        backBufferFormat,
        DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);

    currentBackBuffer = 0;

    // 为每个缓冲区创建RTV标识符
    CD3DX12_CPU_DESCRIPTOR_HANDLE RTVHeapHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (UINT i = 0; i < SwapChainBufferCount; i++)
    {
        swapChain->GetBuffer(i, IID_PPV_ARGS(&swapChainBuffer[i]));
        d3dDevice->CreateRenderTargetView(swapChainBuffer[i].Get(), nullptr, RTVHeapHandle);
        //将RTV描述符堆句柄向后偏移一位
        RTVHeapHandle.Offset(1, rtvDescriptorSize);
    }
    
    // 为深度缓冲区创建DSV标识符
    D3D12_RESOURCE_DESC DSD;
    DSD.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    DSD.Format = depthStencilFormat;
    DSD.MipLevels = 1;
    DSD.Alignment = 0;
    DSD.DepthOrArraySize = 1;
    DSD.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    DSD.Width = clientWidth;
    DSD.Height = clientHeight;
    DSD.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    DSD.SampleDesc.Count = 1;//多重采样采样数量
    DSD.SampleDesc.Quality = 0;//多重采样质量级别

    // 指定深度缓冲区的初始化清除值
    D3D12_CLEAR_VALUE OptiClear;
    OptiClear.Format = depthStencilFormat;
    OptiClear.DepthStencil.Depth = 1.0f;
    OptiClear.DepthStencil.Stencil = 0;
    
    ThrowIfFailed(d3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &DSD, D3D12_RESOURCE_STATE_COMMON,
        &OptiClear, IID_PPV_ARGS(depthStencilBuffer.GetAddressOf())));

    d3dDevice->CreateDepthStencilView(depthStencilBuffer.Get(),
                                        nullptr, 
                                        DepthStencilBufferView());

    commandList->ResourceBarrier(1,&CD3DX12_RESOURCE_BARRIER::Transition(depthStencilBuffer.Get(),
                                    D3D12_RESOURCE_STATE_COMMON,
                                    D3D12_RESOURCE_STATE_DEPTH_WRITE));

    ThrowIfFailed(commandList->Close());
    ID3D12CommandList* CmdList[] = { commandList.Get() };
    commandQueue->ExecuteCommandLists(_countof(CmdList),CmdList);
    FlushCommandQueue();
}

// 处理鼠标信息（可被派生类覆写）
void D3D12RenderHost::KeyboardMsgProc(UINT vk, bool pressed){ return; }
void D3D12RenderHost::MouseDown(WPARAM ButtonState, int x, int y){ return; }
void D3D12RenderHost::MouseUp(WPARAM ButtonState, int x, int y){ return; }
void D3D12RenderHost::MouseMove(WPARAM ButtonState, int x, int y){ return; }
void D3D12RenderHost::MouseWheel(short zDelta) { return; }

// 获取D3D12RenderHost类实例的句柄
HINSTANCE D3D12RenderHost::GetAppInst()const
{
    return this->appInstance;
}
// 获取指向D3D12RenderHost类的指针
D3D12RenderHost* D3D12RenderHost::GetApp()
{
    return appPtr;
}
// 获取程序主窗口句柄
HWND D3D12RenderHost::GetMainHwnd()const
{
    return mainWndHwnd;
}

// 查看是否开启4xMSAA功能
bool D3D12RenderHost::Get4xMSAAState()const
{
    return isMSAA4xOn;
}
// 更改4xMSAA功能开关状态
void D3D12RenderHost::Set4xMSAAState(bool state)
{
    if (isMSAA4xOn != state)
    {
        isMSAA4xOn = state;
    }
}

// 返回缓冲区宽高比
float D3D12RenderHost::GetAspectRatio()const
{
    return static_cast<float>(clientWidth) / static_cast<float>(clientHeight);
}

// 计算每秒帧数和帧渲染时长          
void D3D12RenderHost::UpdateFrameStats()
{
    static int FrameCount = 0;
    static float RenderTime = 0.0f;

    FrameCount++;

    if ((gameTimer.TotalTime() - RenderTime) >= 1.0f)
    {
        fps = (float)FrameCount;
        mspf = 1000.0f / fps;

        FrameCount = 0;
        RenderTime += 1.0f;
    }
}
