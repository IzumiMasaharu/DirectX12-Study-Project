#include "DXApp.h"

using namespace Microsoft::WRL;

DXApp* DXApp::mApp = nullptr;

// 窗口过程回调函数
LRESULT CALLBACK DXAppWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DXApp::GetApp()->MessageProc(hwnd, msg, wParam, lParam);
}

DXApp::DXApp(HINSTANCE hInstance) :appInstance(hInstance)
{
    assert(mApp == nullptr);
    mApp = this;
}
DXApp::~DXApp()
{
    if (d3dDevice != nullptr)
        FlushCommandQueue();
}

// 派生类需覆写此虚函数以编写自己需要的窗口消息处理方式
LRESULT DXApp::MessageProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// 应用运行
int DXApp::Run()
{
    MSG msg = { nullptr };

    gameTimer.Reset();

    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            gameTimer.Tick();

            if (!isAppPaused)
            {
                CalculateFPS_MSPF();
                Update(gameTimer);
                Draw(gameTimer);
            }
            else
            {
                Sleep(100);
            }
        }
    }

    return (int)msg.wParam;
}

// 初始化窗口类
bool DXApp::InitWindowClass(WindowClass& WC, LPCTSTR windowclassName)
{
    // 窗口类命名
    WC.SetWCName(windowclassName);
    // 填写窗口类结构体
    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(wc);
    wc.hInstance = appInstance;
    wc.lpszClassName = WC.GetWCName();
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIconSm = nullptr;
    wc.hIcon = static_cast<HICON>(LoadImage(appInstance, MAKEINTRESOURCE(RENDER), IMAGE_ICON, 512, 512, LR_VGACOLOR));
    wc.hIconSm = nullptr;
    wc.lpfnWndProc = DXAppWndProc;
    wc.lpszMenuName = nullptr;
    wc.style = CS_HREDRAW | CS_VREDRAW;

    // 注册所填写窗口类
    if (!RegisterClassEx(&wc))
    {
        MessageBox(nullptr, L"RegisterWindowClass Failed.", nullptr, 0);
        return false;
    }

    return true;
}

// 初始化窗口（两种重载形式）
// 无指定大小的窗口
bool DXApp::InitWindow(DXApp::Window& Wnd, DXApp::WindowClass WC, const LPCTSTR pWndName)
{
    Wnd.SetWndName(pWndName);

    RECT R = { 0, 0, clientWidth, clientHeight };
    AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
    int width = R.right - R.left;
    int height = R.bottom - R.top;
    // 创建窗口
    Wnd.wndHwnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT, WC.GetWCName(),
        Wnd.windowName, WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU | WS_THICKFRAME | WS_VSCROLL,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        nullptr, nullptr, WC.GetInstance(), nullptr);

    if (!Wnd.wndHwnd)
    {
        MessageBox(nullptr, L"CreateWindow Failed.", nullptr, 0);
        return false;
    }
    else
    {
        SetLayeredWindowAttributes(Wnd.wndHwnd, RGB(0, 0, 0), 255, LWA_COLORKEY);
    }
    if (!mainWndHwnd)
        mainWndHwnd = Wnd.wndHwnd;

    // 展示窗口
    ShowWindow(mainWndHwnd, SW_SHOW);
    UpdateWindow(mainWndHwnd);

    return true;
}
// 指定大小的窗口
bool DXApp::InitWindow(DXApp::Window& Wnd, DXApp::WindowClass WC, const LPCTSTR pWndName, int x, int y, int wx, int wy)
{
    // 设置窗口名称与位置
    Wnd.SetWndName(pWndName);
    Wnd.SetWndPos(x, y, wx, wy);

    RECT R = { 0, 0, Wnd.windowWidth, Wnd.windowHeight };
    AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
    // 创建窗口
    Wnd.wndHwnd = CreateWindowEx(WS_EX_NOREDIRECTIONBITMAP | WS_EX_TOPMOST, WC.GetWCName(),
        Wnd.windowName, WS_OVERLAPPEDWINDOW,
        Wnd.windowX, Wnd.windowY, Wnd.windowWidth, Wnd.windowHeight, nullptr, nullptr, WC.GetInstance(), nullptr);

    if (!Wnd.wndHwnd)
    {
        MessageBox(nullptr, L"CreateWindow Failed.", nullptr, 0);
        return false;
    }

    if (!mainWndHwnd)
        mainWndHwnd = Wnd.wndHwnd;

    /*DWM_BLURBEHIND db{};
    db.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
    db.hRgnBlur = CreateRectRgn(0, 0, -1, -1);
    db.fEnable = true;
    DwmEnableBlurBehindWindow(mainWndHwnd, &db);

    SetWindowLong(mainWndHwnd, GWL_EXSTYLE, GetWindowLong(mainWndHwnd,GWL_EXSTYLE));*/

    // 展示窗口
    ShowWindow(mainWndHwnd, SW_SHOW);
    UpdateWindow(mainWndHwnd);

    return true;
}

bool DXApp::SetTrans()
{
    /*BOOL dwmEnabled = false;
    DwmIsCompositionEnabled(&dwmEnabled);

    MARGINS margins = { -1 };
    DwmExtendFrameIntoClientArea(mainWndHwnd, &margins);

    DWMNCRENDERINGPOLICY policy = DWMNCRP_ENABLED;
    DwmSetWindowAttribute(mainWndHwnd, DWMWA_NCRENDERING_POLICY, &policy, sizeof(policy));*/

    return true;
}

// 初始化DirectX 3D
bool DXApp::InitDirectX3D()
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
ThrowIfFailed(CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory)))
// 创建硬件D3D设备
HRESULT hardwareResulte = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3dDevice));
// 若创建失败，回退至WARP设备
if (FAILED(hardwareResulte))
{
    ComPtr<IDXGIAdapter> pWARPAdapter;
    ThrowIfFailed(dxgiFactory->EnumAdapters(0, &pWARPAdapter))
        ThrowIfFailed(D3D12CreateDevice(pWARPAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3dDevice)))
}
// 创建围栏
ThrowIfFailed(d3dDevice->CreateFence(currentFenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)))

// 检测MSAA级别支持
D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS mQualityLevel;
mQualityLevel.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
mQualityLevel.Format = backBufferFormat;
mQualityLevel.NumQualityLevels = 0;
mQualityLevel.SampleCount = 4;
ThrowIfFailed(d3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &mQualityLevel, sizeof(mQualityLevel)))
MSAA4xQualityLevel = mQualityLevel.NumQualityLevels;
assert(MSAA4xQualityLevel > 0 && "当前MSAA级别不可用");

// 获取描述符大小
rtvDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
dsvDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
cbs_srv_uavDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

#ifdef _DEBUG
//LogAdapters();
#endif
CreateCmdObjects();
CreateSwapChain();
Create_DSV_RTV_DescriptorHeaps();

return true;
}

// 创建命令队列、命令分配器、命令列表
void DXApp::CreateCmdObjects()
{
    // 填写描述命令队列的结构体
    D3D12_COMMAND_QUEUE_DESC qd = {};
    qd.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    qd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    // 创建
    ThrowIfFailed(d3dDevice->CreateCommandQueue(&qd, IID_PPV_ARGS(&commandQueue)))
        ThrowIfFailed(d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(commandAllocator.GetAddressOf())))
        ThrowIfFailed(d3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(commandList.GetAddressOf())))

        commandList->Close();

    ThrowIfFailed(DCompositionCreateDevice(nullptr, __uuidof(IDCompositionDevice), (void**)&dcompDevice));
    ThrowIfFailed(dcompDevice->CreateTargetForHwnd(mainWndHwnd, TRUE, &dcompTarget));
    ThrowIfFailed(dcompDevice->CreateVisual(&dcompVisual));
}

// 创建交换链
void DXApp::CreateSwapChain()
{
    swapChain.Reset();
    // 描述结构体
    DXGI_SWAP_CHAIN_DESC1 scd = {};
    scd.Width = clientWidth;
    scd.Height = clientHeight;
    scd.Format = backBufferFormat;
    scd.Scaling = DXGI_SCALING_STRETCH;
    scd.SampleDesc.Count = 1;
    scd.SampleDesc.Quality = 0;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.BufferCount = SwapChainBufferCount;
    scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    scd.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

    // 创建交换链
    ThrowIfFailed(dxgiFactory->CreateSwapChainForComposition(commandQueue.Get(), &scd, nullptr, swapChain.GetAddressOf()));

    ThrowIfFailed(dcompVisual->SetContent(swapChain.Get()));
    ThrowIfFailed(dcompTarget->SetRoot(dcompVisual.Get()));
}

// 创建描述符堆(RTV和DSV)
void DXApp::Create_DSV_RTV_DescriptorHeaps()
{
    // 创建RTV描述符堆
    D3D12_DESCRIPTOR_HEAP_DESC RTVHeapDesc;
    RTVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    RTVHeapDesc.NodeMask = 0;
    RTVHeapDesc.NumDescriptors = SwapChainBufferCount;
    RTVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&RTVHeapDesc, IID_PPV_ARGS(rtvHeap.GetAddressOf())))
        // 创建DSV描述符堆
        D3D12_DESCRIPTOR_HEAP_DESC DSVHeapDesc;
    DSVHeapDesc.NumDescriptors = 1;
    DSVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    DSVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    DSVHeapDesc.NodeMask = 0;
    ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&DSVHeapDesc, IID_PPV_ARGS(dsvHeap.GetAddressOf())))
}

// 加载枚举所有显示适配器
void DXApp::LogAdapters()
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
void DXApp::LogAdapterOutputs(IDXGIAdapter* adapter)
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
void DXApp::LogAdapterDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
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
void DXApp::FlushCommandQueue()
{
    // 围栏法
    currentFenceValue++;
    ThrowIfFailed(commandQueue->Signal(fence.Get(), currentFenceValue))
        if (fence->GetCompletedValue() < currentFenceValue)
        {
            HANDLE EventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);

            ThrowIfFailed(fence->SetEventOnCompletion(currentFenceValue, EventHandle))

                if (EventHandle)
                {
                    WaitForSingleObject(EventHandle, INFINITE);
                    CloseHandle(EventHandle);
                }
        }
}

// 获取指向当前缓冲区的指针
ID3D12Resource* DXApp::CurrentBackBuffer()const
{
    return swapChainBuffer[currentBackBuffer].Get();
}
// 获取当前后台缓冲区的RTV
D3D12_CPU_DESCRIPTOR_HANDLE DXApp::CurrentBackBufferView()const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
        rtvHeap->GetCPUDescriptorHandleForHeapStart(),
        currentBackBuffer, rtvDescriptorSize);
}
// 获取当前后台缓冲区的DSV
D3D12_CPU_DESCRIPTOR_HANDLE DXApp::DepthStencilBufferView()const
{
    return dsvHeap->GetCPUDescriptorHandleForHeapStart();
}

// 重新设置DX相关尺寸属性
void DXApp::Resize()
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
    DSD.SampleDesc.Count = isMSAA4xOn ? 4 : 1;//多重采样采样数量
    DSD.SampleDesc.Quality = isMSAA4xOn ? (MSAA4xQualityLevel - 1) : 0;//多重采样质量级别

    // 指定深度缓冲区的初始化清除值
    D3D12_CLEAR_VALUE OptiClear;
    OptiClear.Format = depthStencilFormat;
    OptiClear.DepthStencil.Depth = 1.0f;
    OptiClear.DepthStencil.Stencil = 0;

    ThrowIfFailed(d3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &DSD, D3D12_RESOURCE_STATE_COMMON,
        &OptiClear, IID_PPV_ARGS(depthStencilBuffer.GetAddressOf())))

        d3dDevice->CreateDepthStencilView(depthStencilBuffer.Get(),
            nullptr,
            DepthStencilBufferView());

    commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(depthStencilBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_DEPTH_WRITE));

    ThrowIfFailed(commandList->Close())
        ID3D12CommandList* CmdList[] = { commandList.Get() };
    commandQueue->ExecuteCommandLists(_countof(CmdList), CmdList);
    FlushCommandQueue();
}

// 处理鼠标信息（可被派生类覆写）
void DXApp::MouseDown(WPARAM ButtonState, int x, int y) { return; }
void DXApp::MouseUp(WPARAM ButtonState, int x, int y) { return; }
void DXApp::MouseMove(WPARAM ButtonState, int x, int y) { return; }
void DXApp::MouseWheel(short zDelta) { return; }

// 获取DXApp类实例的句柄
HINSTANCE DXApp::GetAppInst()const
{
    return this->appInstance;
}
// 获取指向DXApp类的指针
DXApp* DXApp::GetApp()
{
    return mApp;
}
// 获取程序主窗口句柄
HWND DXApp::GetMainHwnd()const
{
    return mainWndHwnd;
}

// 查看是否开启4xMSAA功能
bool DXApp::Get4xMSAAState()const
{
    return isMSAA4xOn;
}
// 更改4xMSAA功能开关状态
void DXApp::Set4xMSAAState(bool On_Off)
{
    if (isMSAA4xOn != On_Off)
    {
        isMSAA4xOn = On_Off;

        // Recreate the swapchain and buffers with new multisample settings.
        CreateSwapChain();
        Resize();
    }
}

// 返回缓冲区宽高比
float DXApp::W_H_Ratio()const
{
    return static_cast<float>(clientWidth) / static_cast<float>(clientHeight);
}

// 计算每秒帧数和帧渲染时长          
void DXApp::CalculateFPS_MSPF()
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

// 构造窗口类
DXApp::WindowClass::WindowClass(HINSTANCE hInstance) :wndClassInstance(hInstance)
{
}
// 注销窗口类
DXApp::WindowClass::~WindowClass()
{
    UnregisterClass(windowClassName, GetInstance());
}
// 设置窗口类名称
const wchar_t* DXApp::WindowClass::SetWCName(LPCTSTR WCName)
{
    windowClassName = WCName;
    return windowClassName;
}
// 返回窗口类名称
const wchar_t* DXApp::WindowClass::GetWCName()const
{
    return windowClassName;
}
// 返回窗口类实例句柄
HINSTANCE DXApp::WindowClass::GetInstance()const
{
    return wndClassInstance;
}

// 销毁窗口
DXApp::Window::~Window()
{
    if (wndHwnd != nullptr)
        DestroyWindow(wndHwnd);
}
// 设置窗口名称
const wchar_t* DXApp::Window::SetWndName(LPCTSTR WndName)
{
    windowName = WndName;
    return windowName;
}
// 设置窗口坐标数据
void DXApp::Window::SetWndPos(int x, int y, int wx, int wy)
{
    windowX = x;
    windowY = y;
    windowWidth = wx;
    windowHeight = wy;
}
// 获取窗口句柄
HWND DXApp::Window::GetWndHwnd()const
{
    return this->wndHwnd;
}