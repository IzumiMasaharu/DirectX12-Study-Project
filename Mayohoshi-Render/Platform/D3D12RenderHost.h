#include "WindowsManager.h"
#include "GameTimer.h"
#include "D3D12Utility.h"

class D3D12RenderHost : public HandlerWndMsgProc
{
public:
	explicit D3D12RenderHost(HINSTANCE hInstance);
	D3D12RenderHost(const D3D12RenderHost& rhs) = delete;
	D3D12RenderHost operator=(const D3D12RenderHost& rhs) = delete;
	virtual ~D3D12RenderHost();

	int RunMessagePump() const;
	virtual LRESULT wndMsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)override;

	virtual bool Initialize() = 0;
	void ControlLoop();
	virtual void RenderLoop() = 0;
protected:
	bool InitializeD3D12Device();//D3D初始化
	void LogAdapters();//加载枚举所有显示适配器
	void LogAdapterOutputs(IDXGIAdapter* adapter);//加载枚举所有显示输出
	void LogAdapterDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);//加载枚举所有显示输出格式
	void CreateCommandObjects();//创建命令队列、命令分配器、命令列表
	void CreateSwapChain();//创建交换链
	void CreateSwapChainDescriptorHeaps();//创建描述符堆(RTV和DSV)
	
	void FlushCommandQueue();//刷新命令队列
	ID3D12Resource* CurrentBackBuffer()const;//获取指向当前缓冲区的指针
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView()const;//获取当前后台缓冲区的RTV
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilBufferView()const;//获取当前后台缓冲区的DSV

	virtual void Resize();
private:
	virtual void Update(const GameTimer& GTimer) = 0;
	virtual void Draw(const GameTimer& GTimer) = 0;

	virtual void KeyboardMsgProc(UINT vk, bool pressed);
	virtual void MouseDown(WPARAM ButtonState, int x, int y);
	virtual void MouseUp(WPARAM ButtonState, int x, int y);
	virtual void MouseMove(WPARAM ButtonState, int x, int y);
	virtual void MouseWheel(short zDelta);
public:
	HINSTANCE GetAppInst()const;		// 获取应用程序句柄
	static D3D12RenderHost* GetApp();				// 获取指向D3D12RenderHost类的指针
	HWND GetMainHwnd()const;			// 获取程序主窗口句柄
	bool Get4xMSAAState()const;			// 查看是否开启4xMSAA功能
	void Set4xMSAAState(bool On_Off);	// 更改4xMSAA功能开关状态
	float GetAspectRatio()const;				// 返回缓冲区宽高比
	void UpdateFrameStats();			// 计算每秒帧数和帧渲染时长
protected:
	static D3D12RenderHost* appPtr;// 指向D3D12RenderHost类的指针

	GameTimer gameTimer;
	float fps = 0.0f;
	float mspf = 0.0f;

	WindowsManager windowsManager;

	std::thread controlThread;
	std::atomic<bool> isAppRunning{ false };
	std::atomic<bool> isAppPaused{ false };

	std::thread renderThread;
	std::mutex renderMutex;
	std::condition_variable renderCV;
	std::atomic<bool> isFrameReady{ false };
	std::atomic<bool> isFrameRendered{ true };
	std::atomic<bool> isRenderThreadRunning{ false };
	std::atomic<bool> isRenderPaused{ false };

	HINSTANCE appInstance = nullptr;	// 应用程序实例句柄
	HWND mainWndHwnd = nullptr;			// 指向程序窗口的句柄（一般指向主窗口）
	bool isWindowMinimized = false;		// 是否最小化
	bool isWindowMaximized = false;		// 是否最大化
	bool isWindowFullScreen = false;	// 是否全屏

	Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;	//Factory接口指针（Factory接口提供了一套创建DXGI的方法）
	Microsoft::WRL::ComPtr<ID3D12Device> d3dDevice;		//D3D设备指针
	Microsoft::WRL::ComPtr<ID3D12Fence> fence;			//围栏指针
	UINT64 currentFenceValue = 0;						//指示当前围栏值

	bool isMSAA4xOn = false;//是否开启4xMSAA抗锯齿技术
	UINT MSAA4xQualityLevel = 0;//4xMSAA抗锯齿质量级别
    
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;//命令队列指针
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;//命令分配器指针
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;//命令列表指针

	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;//交换链指针
	static const int SwapChainBufferCount = 2;//交换链缓冲区数量
	int currentBackBuffer = 0;//当前后台缓冲区编号
	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, SwapChainBufferCount> swapChainBuffer;//交换链缓冲区指针
	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilBuffer; // 深度/模板缓冲区指针

	UINT rtvDescriptorSize = 0;// RTV描述符大小
	UINT dsvDescriptorSize = 0;// DSV描述符大小
	UINT cbs_srv_uavDescriptorSize = 0;//CBV、SRV、UAV描述符大小
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap;//RTV描述符堆指针
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap;//DSV描述符堆指针

	D3D12_VIEWPORT screenViewport = {};//视口
	D3D12_RECT scissorRect = {};//裁剪矩形

	// 以下变量可在派生类中自行定义
	WindowClassDesc wndClassDesc;
	WindowDesc wndDesc;
	
	D3D_DRIVER_TYPE d3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
	DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	int clientWidth = 640;
	int clientHeight = 360;
};
