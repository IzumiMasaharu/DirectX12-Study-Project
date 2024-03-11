#include "DXApp.h"

using namespace Microsoft::WRL;

DXApp* DXApp::mApp = nullptr;

//窗口过程回调函数
LRESULT CALLBACK DXAppWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DXApp::GetApp()->MessageProc(hwnd, msg, wParam, lParam);
}

DXApp::DXApp(HINSTANCE hInstance):mhAppInst(hInstance)
{
    assert(mApp == nullptr);
    mApp = this;
}
DXApp::~DXApp()
{
    if (md3dDevice != nullptr)
        FlushCommandQueue();
}
//派生类需覆写此虚函数以编写自己需要的窗口消息处理方式
LRESULT DXApp::MessageProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
//应用运行
int DXApp::Run()
{
    MSG msg = { nullptr };

    mGameTimer.Reset();

    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            mGameTimer.Tick();

            if (!mAppPaused)
            {
                CalculateFPS_MSPF();
                Update(mGameTimer);
                Draw(mGameTimer);
            }
            else
            {
                Sleep(100);
            }
        }
    }

    return (int)msg.wParam;
}
//初始化窗口类
bool DXApp::InitWindowClass(WindowClass& WC, LPCTSTR windowclassName)
{
    //窗口类命名
    WC.SetWCName(windowclassName);
    //填写窗口类结构体
    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(wc);
    wc.hInstance = mhAppInst;
    wc.lpszClassName = WC.GetWCName();
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIconSm = nullptr;
    wc.hIcon = static_cast<HICON>(LoadImage(mhAppInst, MAKEINTRESOURCE(CAT_RANA), IMAGE_ICON, 512, 512, LR_VGACOLOR));
    wc.hIconSm = nullptr;
    wc.lpfnWndProc = DXAppWndProc;
    wc.lpszMenuName = nullptr;
    wc.style = CS_HREDRAW | CS_VREDRAW;

    //注册所填写窗口类
    if (!RegisterClassEx(&wc))
    {
        MessageBox(nullptr, L"RegisterWindowClass Failed.", nullptr, 0);
        return false;
    }

    return true;
}
//初始化窗口（两种重载形式）
//无指定大小的窗口
bool DXApp::InitWindow(DXApp::Window& Wnd,DXApp::WindowClass WC,const LPCTSTR pWndName)
{
    Wnd.SetWndName(pWndName);

    RECT R = { 0, 0, mClientWidth, mClientHeight };
    AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
    int width = R.right - R.left;
    int height = R.bottom - R.top;
    //创建窗口
    Wnd.wndHwnd = CreateWindowEx(0, WC.GetWCName(),
        Wnd.pWindowName, WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU | WS_THICKFRAME | WS_VSCROLL,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        nullptr , nullptr, WC.GetInstance(),nullptr);

    if (!Wnd.wndHwnd)
    {
        MessageBox(nullptr, L"CreateWindow Failed.", nullptr, 0);
        return false;
    }
    if (!mhWndHwnd)
        mhWndHwnd = Wnd.wndHwnd;

    //展示窗口
    ShowWindow(mhWndHwnd, SW_SHOW);
    UpdateWindow(mhWndHwnd);
    
    return true;
}
//指定大小的窗口
bool DXApp::InitWindow(DXApp::Window& Wnd,DXApp::WindowClass WC, const LPCTSTR pWndName,
    int x, int y, int wx, int wy)
{
    //设置窗口名称与位置
    Wnd.SetWndName(pWndName);
    Wnd.SetWndPos(x, y, wx, wy);

    RECT R = { 0, 0, Wnd.mWin_wx, Wnd.mWin_wy };
    AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
    //创建窗口
    Wnd.wndHwnd = CreateWindowEx(0, WC.GetWCName(),
        Wnd.pWindowName, WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU | WS_THICKFRAME | WS_VSCROLL,
        Wnd.mWin_x, Wnd.mWin_y, Wnd.mWin_wx, Wnd.mWin_wy,nullptr, nullptr, WC.GetInstance(), nullptr);
    
    if (!Wnd.wndHwnd)
    {
        MessageBox(nullptr, L"CreateWindow Failed.", nullptr, 0);
        return false;
    }
    if (!mhWndHwnd) 
        mhWndHwnd = Wnd.wndHwnd;
    //展示窗口
    ShowWindow(mhWndHwnd, SW_SHOW);
    UpdateWindow(mhWndHwnd);

    return true;
}
//初始化DirectX 3D
bool DXApp::InitDirectX3D()
{
    //启动D3D调试层
    #if defined(DEBUG)||defined(_DEBUG)
    {
        ComPtr<ID3D12Debug> debugCotroller;
        ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugCotroller)));
        debugCotroller->EnableDebugLayer();
    }
    #endif
    //创建DXGI Factory
    ThrowIfFailed(CreateDXGIFactory(IID_PPV_ARGS(&mdxgiFactory)))
    //创建硬件D3D设备
    HRESULT hardwareResulte = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&md3dDevice));
    //若创建失败，回退至WARP设备
    if (FAILED(hardwareResulte))
    {
        ComPtr<IDXGIAdapter> pWARPAdapter;
        ThrowIfFailed(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWARPAdapter)))
        ThrowIfFailed(D3D12CreateDevice(pWARPAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&md3dDevice)))
    }
    //创建围栏
    ThrowIfFailed(md3dDevice->CreateFence(mCurrentFence, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)))
    
    //检测MSAA级别支持
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS mQualityLevel;
    mQualityLevel.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    mQualityLevel.Format = mBackBufferFormat;
    mQualityLevel.NumQualityLevels = 0;
    mQualityLevel.SampleCount = 4;
    ThrowIfFailed(md3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, 
                     &mQualityLevel, 
                     sizeof(mQualityLevel)))
    m4xMSAAQuality = mQualityLevel.NumQualityLevels;
    assert(m4xMSAAQuality > 0 && "当前MSAA级别不可用");

    //获取描述符大小
    mRTVDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    mDSVDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    mCBV_SRV_UAVDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

#ifdef _DEBUG
    LogAdapters();
#endif
    CreateCmdObjects();
    CreateSwapChain();
    Create_DSV_RTV_DescriptorHeaps();

    return true;
}
//创建命令队列、命令分配器、命令列表
void DXApp::CreateCmdObjects()
{
    //填写描述命令队列的结构体
    D3D12_COMMAND_QUEUE_DESC qd = {};
    qd.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    qd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    //创建
    ThrowIfFailed(md3dDevice->CreateCommandQueue(&qd, IID_PPV_ARGS(&mCommandQueue)))
    ThrowIfFailed(md3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS( mCommandAllocator.GetAddressOf())))
    ThrowIfFailed(md3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator.Get(), 
                                    nullptr, IID_PPV_ARGS(mCommandList.GetAddressOf())))

    mCommandList->Close();
}
//创建交换链
void DXApp::CreateSwapChain()
{
    //释放之前所创建的的交换链，重新创建
    mSwapChain.Reset();
    //填写描述所创建交换链的结构体
    DXGI_SWAP_CHAIN_DESC scd;
    scd.BufferDesc.Width = mClientWidth;
    scd.BufferDesc.Height = mClientHeight;
    scd.BufferDesc.Format = mBackBufferFormat;//显示格式
    scd.BufferDesc.RefreshRate.Numerator = 120;//最高刷新率
    scd.BufferDesc.RefreshRate.Denominator = 1;//最低刷新率
    scd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;//是否缩放
    scd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;//逐行扫描还是隔行扫描
    scd.SampleDesc.Count = m4xMSAAState ? 4 : 1;//多重采样采样数量
    scd.SampleDesc.Quality = m4xMSAAState ? (m4xMSAAQuality - 1) : 0;//多重采样质量级别
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.BufferCount = SwapChainBufferCount;
    scd.Windowed = true;//窗口显示还是全屏显示
    scd.OutputWindow = mhWndHwnd;//图像输出窗口句柄
    scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    ThrowIfFailed(mdxgiFactory->CreateSwapChain(mCommandQueue.Get(), &scd, mSwapChain.GetAddressOf()))
}
//创建描述符堆(RTV和DSV)
void DXApp::Create_DSV_RTV_DescriptorHeaps()
{
    //创建RTV描述符堆
    D3D12_DESCRIPTOR_HEAP_DESC RTVHeapDesc ;
    RTVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    RTVHeapDesc.NodeMask = 0;
    RTVHeapDesc.NumDescriptors = SwapChainBufferCount;
    RTVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&RTVHeapDesc, IID_PPV_ARGS(mRTVHeap.GetAddressOf())))
    //创建DSV描述符堆
    D3D12_DESCRIPTOR_HEAP_DESC DSVHeapDesc ;
    DSVHeapDesc.NumDescriptors = 1;
    DSVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    DSVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    DSVHeapDesc.NodeMask = 0;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&DSVHeapDesc, IID_PPV_ARGS(mDSVHeap.GetAddressOf())))
}
//加载枚举所有显示适配器
void DXApp::LogAdapters()
{
    IDXGIAdapter* Adapter = nullptr;
    std::vector<IDXGIAdapter*> AdapterList;
    for (UINT i = 0; mdxgiFactory->EnumAdapters(i, &Adapter) != DXGI_ERROR_NOT_FOUND; i++)
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
//加载枚举所有显示输出
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
//加载枚举所有显示输出格式
void DXApp::LogAdapterDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
{
    UINT count = 0;
    UINT flags = 0;

    output->GetDisplayModeList(format, flags, &count, nullptr);//将参数pDesc设为nullptr可获取满足条件的显示模式的数量，并存入count中
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
//刷新命令队列
void DXApp::FlushCommandQueue()
{
    mCurrentFence++;
    ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mCurrentFence))
    if (mFence->GetCompletedValue() < mCurrentFence)
    {
        HANDLE EventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);

        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFence, EventHandle))

        if (EventHandle)
        {
            WaitForSingleObject(EventHandle, INFINITE);
            CloseHandle(EventHandle);
        }
    }
}
//获取指向当前缓冲区的指针
ID3D12Resource* DXApp::CurrentBackBuffer()const
{
    return mSwapChainBuffer[mCurrentBackBuffer].Get();
}
//获取当前后台缓冲区的RTV
D3D12_CPU_DESCRIPTOR_HANDLE DXApp::CurrentBackBufferView()const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
        mRTVHeap->GetCPUDescriptorHandleForHeapStart(),
        mCurrentBackBuffer, mRTVDescriptorSize);
}
//获取当前后台缓冲区的DSV
D3D12_CPU_DESCRIPTOR_HANDLE DXApp::DepthStencilBufferView()const
{
    return mDSVHeap->GetCPUDescriptorHandleForHeapStart();
}

//重新设置DX相关尺寸属性
void DXApp::Resize()
{
    assert(md3dDevice);
    assert(mCommandAllocator);
    assert(mSwapChain);

    FlushCommandQueue();

    mCommandList->Reset(mCommandAllocator.Get(), nullptr);

    for (UINT i = 0; i < SwapChainBufferCount; i++)
        mSwapChainBuffer[i].Reset();
    mDepthStencilBuffer.Reset();

    mSwapChain->ResizeBuffers(
        SwapChainBufferCount,
        mClientWidth, mClientHeight,
        mBackBufferFormat,
        DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);

    mCurrentBackBuffer = 0;

    //为每个缓冲区创建RTV标识符
    CD3DX12_CPU_DESCRIPTOR_HANDLE RTVHeapHandle(mRTVHeap->GetCPUDescriptorHandleForHeapStart());
    for (UINT i = 0; i < SwapChainBufferCount; i++)
    {
        mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i]));
        md3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, RTVHeapHandle);
        //将RTV描述符堆句柄向后偏移一位
        RTVHeapHandle.Offset(1, mRTVDescriptorSize);
    }
    //为每个缓冲区创建DSV标识符
    D3D12_RESOURCE_DESC DSD;
    DSD.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    DSD.Format = mDepthStencilFormat;
    DSD.MipLevels = 1;
    DSD.Alignment = 0;
    DSD.DepthOrArraySize = 1;
    DSD.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    DSD.Width = mClientWidth;
    DSD.Height = mClientHeight;
    DSD.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    DSD.SampleDesc.Count = m4xMSAAState ? 4 : 1;//多重采样采样数量
    DSD.SampleDesc.Quality = m4xMSAAState ? (m4xMSAAQuality - 1) : 0;//多重采样质量级别

    D3D12_CLEAR_VALUE OptiClear;
    OptiClear.Format = mDepthStencilFormat;
    OptiClear.DepthStencil.Depth = 1.0f;
    OptiClear.DepthStencil.Stencil = 0;
    
    ThrowIfFailed(md3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &DSD, D3D12_RESOURCE_STATE_COMMON,
        &OptiClear, IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())))

    md3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(),
                                        nullptr, 
                                        DepthStencilBufferView());
    
    mCommandList->ResourceBarrier(1,&CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(),
                                    D3D12_RESOURCE_STATE_COMMON,
                                    D3D12_RESOURCE_STATE_DEPTH_WRITE));

    ThrowIfFailed(mCommandList->Close())
    ID3D12CommandList* CmdList[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(CmdList),CmdList);
    FlushCommandQueue();
}
//处理鼠标信息（可被派生类覆写）
void DXApp::MouseDown(WPARAM ButtonState, int x, int y){ return; }
void DXApp::MouseUp(WPARAM ButtonState, int x, int y){ return; }
void DXApp::MouseMove(WPARAM ButtonState, int x, int y){ return; }
void DXApp::MouseWheel(short zDelta) { return; }

//获取DXApp类实例的句柄
HINSTANCE DXApp::GetAppInst()const
{
    return this->mhAppInst;
}
//获取指向DXApp类的指针
DXApp* DXApp::GetApp()
{
    return mApp;
}
//获取程序主窗口句柄
HWND DXApp::GetMainHwnd()const
{
    return mhWndHwnd;
}
//查看是否开启4xMSAA功能
bool DXApp::Get4xMSAAState()const
{
    return m4xMSAAState;
}
//更改4xMSAA功能开关状态
void DXApp::Set4xMSAAState(bool On_Off)
{
    if (m4xMSAAState != On_Off)
    {
        m4xMSAAState = On_Off;

        // Recreate the swapchain and buffers with new multisample settings.
        CreateSwapChain();
        Resize();
    }
}
//返回缓冲区宽高比
float DXApp::W_H_Ratio()const
{
    return static_cast<float>(mClientWidth) / static_cast<float>(mClientHeight);
}
//计算每秒帧数和帧渲染时长          
void DXApp::CalculateFPS_MSPF()
{
    static int FrameCount = 0;
    static float RenderTime = 0.0f;

    FrameCount++;

    if ((mGameTimer.TotalTime() - RenderTime) >= 1.0f)
    {
        FPS = (float)FrameCount;
        MSPF = 1000.0f / FPS;

        FrameCount = 0;
        RenderTime += 1.0f;
    }
}

//构造窗口类
DXApp::WindowClass::WindowClass(HINSTANCE hInstance) :hWndClassInst(hInstance)
{}
//注销窗口类
DXApp::WindowClass::~WindowClass()
{
    UnregisterClass(windowclassName, GetInstance());
}
//设置窗口类名称
const wchar_t* DXApp::WindowClass::SetWCName(LPCTSTR WCName)
{
    windowclassName = WCName;
    return windowclassName;
}
//返回窗口类名称
const wchar_t* DXApp::WindowClass::GetWCName()const
{
    return windowclassName;
}
//返回窗口类实例句柄
HINSTANCE DXApp::WindowClass::GetInstance()const
{
    return hWndClassInst;
}

//销毁窗口
DXApp::Window::~Window()
{
    if (wndHwnd != nullptr)
        DestroyWindow(wndHwnd);
}
//设置窗口名称
const wchar_t* DXApp::Window::SetWndName(LPCTSTR WndName)
{
    pWindowName = WndName;
    return pWindowName;
}
//设置窗口坐标数据
void DXApp::Window::SetWndPos(int x, int y, int wx, int wy)
{
    mWin_x = x;
    mWin_y = y;
    mWin_wx = wx;
    mWin_wy = wy;
}
//获取窗口句柄
HWND DXApp::Window::GetWndHwnd()const
{
    return this->wndHwnd;
}