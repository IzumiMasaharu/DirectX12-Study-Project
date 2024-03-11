#include "DXBase.h"
#include "GameTimer.h"

class DXApp
{
protected:
	class WindowClass;
	class Window;
protected:
	explicit DXApp(HINSTANCE hInstance);
	DXApp(const DXApp& rhs) = delete;
	virtual ~DXApp();
protected:
	DXApp operator=(const DXApp& rhs) = delete;
public:
	virtual LRESULT MessageProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);//消息过程处理函数（需通过派生类覆写）
	virtual bool Init() = 0;
	int Run();
protected:
	//初始化实现
	bool InitWindowClass(WindowClass& WC,LPCTSTR windowclassName);//窗口类初始化
	bool InitWindow(DXApp::Window& Wnd, DXApp::WindowClass WC, const LPCTSTR pWndName);//窗口初始化重载1
	bool InitWindow(DXApp::Window& Wnd, DXApp::WindowClass WC, const LPCTSTR pWndName,
		int x, int y, int wx, int wy);//窗口初始化重载2
	bool InitDirectX3D();//D3D初始化
	void LogAdapters();//加载枚举所有显示适配器
	void LogAdapterOutputs(IDXGIAdapter* adapter);//加载枚举所有显示输出
	void LogAdapterDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);//加载枚举所有显示输出格式
	void CreateCmdObjects();//创建命令队列、命令分配器、命令列表
	void CreateSwapChain();//创建交换链
	void Create_DSV_RTV_DescriptorHeaps();//创建描述符堆(RTV和DSV)
	
	void FlushCommandQueue();//刷新命令队列
	ID3D12Resource* CurrentBackBuffer()const;//获取指向当前缓冲区的指针
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView()const;//获取当前后台缓冲区的RTV
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilBufferView()const;//获取当前后台缓冲区的DSV
protected:
	virtual void Resize();
private:
	virtual void Update(const GameTimer& GTimer) = 0;
	virtual void Draw(const GameTimer& GTimer) = 0;

	virtual void MouseDown(WPARAM ButtonState, int x, int y);
	virtual void MouseUp(WPARAM ButtonState, int x, int y);
	virtual void MouseMove(WPARAM ButtonState, int x, int y);
	virtual void MouseWheel(short zDelta);
public:
	HINSTANCE GetAppInst()const;//获取应用程序句柄
	static DXApp* GetApp();//获取指向DXApp类的指针
	HWND GetMainHwnd()const;//获取程序主窗口句柄
	bool Get4xMSAAState()const;//查看是否开启4xMSAA功能
	void Set4xMSAAState(bool On_Off);//更改4xMSAA功能开关状态
	float W_H_Ratio()const;//返回缓冲区宽高比
	void CalculateFPS_MSPF();//计算每秒帧数和帧渲染时长
protected:
	static DXApp* mApp;//指向DXApp类的指针

	GameTimer mGameTimer;
	float FPS = 0.0f;
	float MSPF = 0.0f;

	HINSTANCE mhAppInst = nullptr;//应用程序实例句柄
	HWND mhWndHwnd = nullptr;//指向程序窗口的句柄（一般指向主窗口）
	bool mAppPaused = false;//应用程序是否暂停
	bool mMinimized = false;//是否最小化
	bool mMaximized = false;//是否最大化
	bool mResized = false;//窗口是否改变大小
	bool mFullScreenState = false;//是否全屏

	Microsoft::WRL::ComPtr<IDXGIFactory4> mdxgiFactory;//Factory接口指针（Factory接口提供了一套创建DXGI的方法）
	Microsoft::WRL::ComPtr<ID3D12Device> md3dDevice;//D3D设备指针
	Microsoft::WRL::ComPtr<ID3D12Fence> mFence;//围栏指针
	UINT64 mCurrentFence = 0;//指示当前围栏值

	bool m4xMSAAState = false;//是否开启4xMSAA抗锯齿技术
	UINT m4xMSAAQuality = 0;//4xMSAA抗锯齿质量级别

	//                   CPU                  ————————>               GPU             //
	//  CommandAllocator ——> CommandList      ————————>           CommandQueue        //     
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;//命令队列指针
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mCommandAllocator;//命令分配器指针
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;//命令列表指针

	Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;//交换链指针
	static const int SwapChainBufferCount = 2;//交换链缓冲区数量
	int mCurrentBackBuffer = 0;//当前后缓冲区编号
	Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];//交换链缓冲区指针
	Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;//深度模板缓冲区指针

	UINT mRTVDescriptorSize = 0;//RTV描述符大小
	UINT mDSVDescriptorSize = 0;//DSV描述符大小
	UINT mCBV_SRV_UAVDescriptorSize = 0;//CBV、SRV、UAV描述符大小
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRTVHeap;//RTV描述符堆指针
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDSVHeap;//DSV描述符堆指针

	D3D12_VIEWPORT mScreenViewport = {};//视口
	D3D12_RECT mScissorRect = {};//裁剪矩形

	//以下变量可在派生类中自行定义
	LPCTSTR mMainWndTitle=L"DefaultTitle";
	D3D_DRIVER_TYPE md3dDriverType= D3D_DRIVER_TYPE_HARDWARE;
	DXGI_FORMAT mBackBufferFormat= DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	int mClientWidth=800;
	int mClientHeight=600;
};
//类：窗口类的声明
class DXApp::WindowClass
{
public:
	WindowClass() = default;
	explicit WindowClass(HINSTANCE hInstance);
	~WindowClass();
public:
	const wchar_t* SetWCName(LPCTSTR WCName);//设置窗口类名称
	const wchar_t* GetWCName()const;//返回窗口类名称
	HINSTANCE GetInstance()const;//返回窗口类实例句柄
private:
	HINSTANCE hWndClassInst;
	const wchar_t* windowclassName = nullptr;
};
//类：窗口的声明
class DXApp::Window
{
public:
	friend bool DXApp::InitWindow(DXApp::Window& Wnd, DXApp::WindowClass WC, const LPCTSTR pWndName);
	friend bool DXApp::InitWindow(DXApp::Window& Wnd, DXApp::WindowClass WC, const LPCTSTR pWndName,
							int x, int y, int wx, int wy);
public:
	Window() = default;
	~Window();
public:
	const wchar_t* SetWndName(LPCTSTR WndName);//设置窗口名称
	void SetWndPos(int x, int y, int wx, int wy);//设置窗口坐标数据
	HWND GetWndHwnd()const;//获取窗口句柄
private:
	HWND wndHwnd = nullptr;
	LPCTSTR pWindowName = nullptr;
	int32_t mWin_x = 0;
	int32_t mWin_y = 0;
	int32_t mWin_wx = 0;
	int32_t mWin_wy = 0;
};