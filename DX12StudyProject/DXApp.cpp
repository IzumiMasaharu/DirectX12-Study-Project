#include "DXApp.h"

using namespace Microsoft::WRL;

DXApp* DXApp::mApp = nullptr;

//���ڹ��̻ص�����
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
//�������踲д���麯���Ա�д�Լ���Ҫ�Ĵ�����Ϣ����ʽ
LRESULT DXApp::MessageProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
//Ӧ������
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
//��ʼ��������
bool DXApp::InitWindowClass(WindowClass& WC, LPCTSTR windowclassName)
{
    //����������
    WC.SetWCName(windowclassName);
    //��д������ṹ��
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

    //ע������д������
    if (!RegisterClassEx(&wc))
    {
        MessageBox(nullptr, L"RegisterWindowClass Failed.", nullptr, 0);
        return false;
    }

    return true;
}
//��ʼ�����ڣ�����������ʽ��
//��ָ����С�Ĵ���
bool DXApp::InitWindow(DXApp::Window& Wnd,DXApp::WindowClass WC,const LPCTSTR pWndName)
{
    Wnd.SetWndName(pWndName);

    RECT R = { 0, 0, mClientWidth, mClientHeight };
    AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
    int width = R.right - R.left;
    int height = R.bottom - R.top;
    //��������
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

    //չʾ����
    ShowWindow(mhWndHwnd, SW_SHOW);
    UpdateWindow(mhWndHwnd);
    
    return true;
}
//ָ����С�Ĵ���
bool DXApp::InitWindow(DXApp::Window& Wnd,DXApp::WindowClass WC, const LPCTSTR pWndName,
    int x, int y, int wx, int wy)
{
    //���ô���������λ��
    Wnd.SetWndName(pWndName);
    Wnd.SetWndPos(x, y, wx, wy);

    RECT R = { 0, 0, Wnd.mWin_wx, Wnd.mWin_wy };
    AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
    //��������
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
    //չʾ����
    ShowWindow(mhWndHwnd, SW_SHOW);
    UpdateWindow(mhWndHwnd);

    return true;
}
//��ʼ��DirectX 3D
bool DXApp::InitDirectX3D()
{
    //����D3D���Բ�
    #if defined(DEBUG)||defined(_DEBUG)
    {
        ComPtr<ID3D12Debug> debugCotroller;
        ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugCotroller)));
        debugCotroller->EnableDebugLayer();
    }
    #endif
    //����DXGI Factory
    ThrowIfFailed(CreateDXGIFactory(IID_PPV_ARGS(&mdxgiFactory)))
    //����Ӳ��D3D�豸
    HRESULT hardwareResulte = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&md3dDevice));
    //������ʧ�ܣ�������WARP�豸
    if (FAILED(hardwareResulte))
    {
        ComPtr<IDXGIAdapter> pWARPAdapter;
        ThrowIfFailed(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWARPAdapter)))
        ThrowIfFailed(D3D12CreateDevice(pWARPAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&md3dDevice)))
    }
    //����Χ��
    ThrowIfFailed(md3dDevice->CreateFence(mCurrentFence, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)))
    
    //���MSAA����֧��
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS mQualityLevel;
    mQualityLevel.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    mQualityLevel.Format = mBackBufferFormat;
    mQualityLevel.NumQualityLevels = 0;
    mQualityLevel.SampleCount = 4;
    ThrowIfFailed(md3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, 
                     &mQualityLevel, 
                     sizeof(mQualityLevel)))
    m4xMSAAQuality = mQualityLevel.NumQualityLevels;
    assert(m4xMSAAQuality > 0 && "��ǰMSAA���𲻿���");

    //��ȡ��������С
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
//����������С�����������������б�
void DXApp::CreateCmdObjects()
{
    //��д����������еĽṹ��
    D3D12_COMMAND_QUEUE_DESC qd = {};
    qd.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    qd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    //����
    ThrowIfFailed(md3dDevice->CreateCommandQueue(&qd, IID_PPV_ARGS(&mCommandQueue)))
    ThrowIfFailed(md3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS( mCommandAllocator.GetAddressOf())))
    ThrowIfFailed(md3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator.Get(), 
                                    nullptr, IID_PPV_ARGS(mCommandList.GetAddressOf())))

    mCommandList->Close();
}
//����������
void DXApp::CreateSwapChain()
{
    //�ͷ�֮ǰ�������ĵĽ����������´���
    mSwapChain.Reset();
    //��д�����������������Ľṹ��
    DXGI_SWAP_CHAIN_DESC scd;
    scd.BufferDesc.Width = mClientWidth;
    scd.BufferDesc.Height = mClientHeight;
    scd.BufferDesc.Format = mBackBufferFormat;//��ʾ��ʽ
    scd.BufferDesc.RefreshRate.Numerator = 120;//���ˢ����
    scd.BufferDesc.RefreshRate.Denominator = 1;//���ˢ����
    scd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;//�Ƿ�����
    scd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;//����ɨ�軹�Ǹ���ɨ��
    scd.SampleDesc.Count = m4xMSAAState ? 4 : 1;//���ز�����������
    scd.SampleDesc.Quality = m4xMSAAState ? (m4xMSAAQuality - 1) : 0;//���ز�����������
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.BufferCount = SwapChainBufferCount;
    scd.Windowed = true;//������ʾ����ȫ����ʾ
    scd.OutputWindow = mhWndHwnd;//ͼ��������ھ��
    scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    ThrowIfFailed(mdxgiFactory->CreateSwapChain(mCommandQueue.Get(), &scd, mSwapChain.GetAddressOf()))
}
//������������(RTV��DSV)
void DXApp::Create_DSV_RTV_DescriptorHeaps()
{
    //����RTV��������
    D3D12_DESCRIPTOR_HEAP_DESC RTVHeapDesc ;
    RTVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    RTVHeapDesc.NodeMask = 0;
    RTVHeapDesc.NumDescriptors = SwapChainBufferCount;
    RTVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&RTVHeapDesc, IID_PPV_ARGS(mRTVHeap.GetAddressOf())))
    //����DSV��������
    D3D12_DESCRIPTOR_HEAP_DESC DSVHeapDesc ;
    DSVHeapDesc.NumDescriptors = 1;
    DSVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    DSVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    DSVHeapDesc.NodeMask = 0;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&DSVHeapDesc, IID_PPV_ARGS(mDSVHeap.GetAddressOf())))
}
//����ö��������ʾ������
void DXApp::LogAdapters()
{
    IDXGIAdapter* Adapter = nullptr;
    std::vector<IDXGIAdapter*> AdapterList;
    for (UINT i = 0; mdxgiFactory->EnumAdapters(i, &Adapter) != DXGI_ERROR_NOT_FOUND; i++)
    {
        DXGI_ADAPTER_DESC ad;
        Adapter->GetDesc(&ad);

        std::wstring AdapterText = L"������ʾ������ " + std::to_wstring(i + 1) + L":";
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
//����ö��������ʾ���
void DXApp::LogAdapterOutputs(IDXGIAdapter* adapter)
{
    IDXGIOutput* Output = nullptr;
    for (UINT i = 0; adapter->EnumOutputs(i, &Output) != DXGI_ERROR_NOT_FOUND; i++)
    {
        DXGI_OUTPUT_DESC od;
        Output->GetDesc(&od);

        std::wstring OutputText = L"������ʾ��� " + std::to_wstring(i + 1) + L":";
        OutputText += od.DeviceName;
        OutputText += L"\n";
        OutputDebugString(OutputText.c_str());

        LogAdapterDisplayModes(Output, DXGI_FORMAT_B8G8R8A8_UNORM);

        ReleaseCom(Output);
    }
}
//����ö��������ʾ�����ʽ
void DXApp::LogAdapterDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
{
    UINT count = 0;
    UINT flags = 0;

    output->GetDisplayModeList(format, flags, &count, nullptr);//������pDesc��Ϊnullptr�ɻ�ȡ������������ʾģʽ��������������count��
    std::vector<DXGI_MODE_DESC> modelist(count);
    output->GetDisplayModeList(format, flags, &count, &modelist[0]);

    uint32_t x = 0;
    for (auto& i : modelist)
    {
        x++;
        UINT nu = i.RefreshRate.Numerator;
        UINT de = i.RefreshRate.Denominator;
        std::wstring DisplayModeText =
            L"��ʾģʽ" + std::to_wstring(x) +
            L"��ȣ�" + std::to_wstring(i.Width) + L"  " +
            L"�߶ȣ�" + std::to_wstring(i.Height) + L"  " +
            L"ˢ���ʣ�" + std::to_wstring(nu) + L"~" + std::to_wstring(de) +
            L"\n";

        OutputDebugString(DisplayModeText.c_str());
    }
}
//ˢ���������
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
//��ȡָ��ǰ��������ָ��
ID3D12Resource* DXApp::CurrentBackBuffer()const
{
    return mSwapChainBuffer[mCurrentBackBuffer].Get();
}
//��ȡ��ǰ��̨��������RTV
D3D12_CPU_DESCRIPTOR_HANDLE DXApp::CurrentBackBufferView()const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
        mRTVHeap->GetCPUDescriptorHandleForHeapStart(),
        mCurrentBackBuffer, mRTVDescriptorSize);
}
//��ȡ��ǰ��̨��������DSV
D3D12_CPU_DESCRIPTOR_HANDLE DXApp::DepthStencilBufferView()const
{
    return mDSVHeap->GetCPUDescriptorHandleForHeapStart();
}

//��������DX��سߴ�����
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

    //Ϊÿ������������RTV��ʶ��
    CD3DX12_CPU_DESCRIPTOR_HANDLE RTVHeapHandle(mRTVHeap->GetCPUDescriptorHandleForHeapStart());
    for (UINT i = 0; i < SwapChainBufferCount; i++)
    {
        mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i]));
        md3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, RTVHeapHandle);
        //��RTV�������Ѿ�����ƫ��һλ
        RTVHeapHandle.Offset(1, mRTVDescriptorSize);
    }
    //Ϊÿ������������DSV��ʶ��
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
    DSD.SampleDesc.Count = m4xMSAAState ? 4 : 1;//���ز�����������
    DSD.SampleDesc.Quality = m4xMSAAState ? (m4xMSAAQuality - 1) : 0;//���ز�����������

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
//���������Ϣ���ɱ������าд��
void DXApp::MouseDown(WPARAM ButtonState, int x, int y){ return; }
void DXApp::MouseUp(WPARAM ButtonState, int x, int y){ return; }
void DXApp::MouseMove(WPARAM ButtonState, int x, int y){ return; }
void DXApp::MouseWheel(short zDelta) { return; }

//��ȡDXApp��ʵ���ľ��
HINSTANCE DXApp::GetAppInst()const
{
    return this->mhAppInst;
}
//��ȡָ��DXApp���ָ��
DXApp* DXApp::GetApp()
{
    return mApp;
}
//��ȡ���������ھ��
HWND DXApp::GetMainHwnd()const
{
    return mhWndHwnd;
}
//�鿴�Ƿ���4xMSAA����
bool DXApp::Get4xMSAAState()const
{
    return m4xMSAAState;
}
//����4xMSAA���ܿ���״̬
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
//���ػ�������߱�
float DXApp::W_H_Ratio()const
{
    return static_cast<float>(mClientWidth) / static_cast<float>(mClientHeight);
}
//����ÿ��֡����֡��Ⱦʱ��          
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

//���촰����
DXApp::WindowClass::WindowClass(HINSTANCE hInstance) :hWndClassInst(hInstance)
{}
//ע��������
DXApp::WindowClass::~WindowClass()
{
    UnregisterClass(windowclassName, GetInstance());
}
//���ô���������
const wchar_t* DXApp::WindowClass::SetWCName(LPCTSTR WCName)
{
    windowclassName = WCName;
    return windowclassName;
}
//���ش���������
const wchar_t* DXApp::WindowClass::GetWCName()const
{
    return windowclassName;
}
//���ش�����ʵ�����
HINSTANCE DXApp::WindowClass::GetInstance()const
{
    return hWndClassInst;
}

//���ٴ���
DXApp::Window::~Window()
{
    if (wndHwnd != nullptr)
        DestroyWindow(wndHwnd);
}
//���ô�������
const wchar_t* DXApp::Window::SetWndName(LPCTSTR WndName)
{
    pWindowName = WndName;
    return pWindowName;
}
//���ô�����������
void DXApp::Window::SetWndPos(int x, int y, int wx, int wy)
{
    mWin_x = x;
    mWin_y = y;
    mWin_wx = wx;
    mWin_wy = wy;
}
//��ȡ���ھ��
HWND DXApp::Window::GetWndHwnd()const
{
    return this->wndHwnd;
}