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
	virtual LRESULT MessageProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);//��Ϣ���̴���������ͨ�������าд��
	virtual bool Init() = 0;
	int Run();
protected:
	//��ʼ��ʵ��
	bool InitWindowClass(WindowClass& WC,LPCTSTR WindowClassName);//�������ʼ��
	bool InitWindow(DXApp::Window& Wnd, DXApp::WindowClass WC, const LPCTSTR pWndName);//���ڳ�ʼ������1
	bool InitWindow(DXApp::Window& Wnd, DXApp::WindowClass WC, const LPCTSTR pWndName,
		int x, int y, int wx, int wy);//���ڳ�ʼ������2
	bool InitDirectX3D();//D3D��ʼ��
	void LogAdapters();//����ö��������ʾ������
	void LogAdapterOutputs(IDXGIAdapter* adapter);//����ö��������ʾ���
	void LogAdapterDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);//����ö��������ʾ�����ʽ
	void CreateCmdObjects();//����������С�����������������б�
	void CreateSwapChain();//����������
	void Create_DSV_RTV_DescriptorHeaps();//������������(RTV��DSV)
	
	void FlushCommandQueue();//ˢ���������
	ID3D12Resource* CurrentBackBuffer()const;//��ȡָ��ǰ��������ָ��
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView()const;//��ȡ��ǰ��̨��������RTV
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilBufferView()const;//��ȡ��ǰ��̨��������DSV
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
	HINSTANCE GetAppInst()const;//��ȡӦ�ó�����
	static DXApp* GetApp();//��ȡָ��DXApp���ָ��
	HWND GetMainHwnd()const;//��ȡ���������ھ��
	bool Get4xMSAAState()const;//�鿴�Ƿ���4xMSAA����
	void Set4xMSAAState(bool On_Off);//����4xMSAA���ܿ���״̬
	float W_H_Ratio()const;//���ػ�������߱�
	void CalculateFPS_MSPF();//����ÿ��֡����֡��Ⱦʱ��
protected:
	static DXApp* mApp;//ָ��DXApp���ָ��

	GameTimer mGameTimer;
	float FPS = 0.0f;
	float MSPF = 0.0f;

	HINSTANCE mhAppInst = nullptr;//Ӧ�ó���ʵ�����
	HWND mhWndHwnd = nullptr;//ָ����򴰿ڵľ����һ��ָ�������ڣ�
	bool mAppPaused = false;//Ӧ�ó����Ƿ���ͣ
	bool mMinimized = false;//�Ƿ���С��
	bool mMaximized = false;//�Ƿ����
	bool mResized = false;//�����Ƿ�ı��С
	bool mFullScreenState = false;//�Ƿ�ȫ��

	Microsoft::WRL::ComPtr<IDXGIFactory4> mdxgiFactory;//Factory�ӿ�ָ�루Factory�ӿ��ṩ��һ�״���DXGI�ķ�����
	Microsoft::WRL::ComPtr<ID3D12Device> md3dDevice;//D3D�豸ָ��
	Microsoft::WRL::ComPtr<ID3D12Fence> mFence;//Χ��ָ��
	UINT64 mCurrentFence = 0;//ָʾ��ǰΧ��ֵ

	bool m4xMSAAState = false;//�Ƿ���4xMSAA����ݼ���
	UINT m4xMSAAQuality = 0;//4xMSAA�������������

	//                   CPU                  ����������������>               GPU             //
	//  CommandAllocator ����> CommandList      ����������������>           CommandQueue        //     
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;//�������ָ��
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mCommandAllocator;//���������ָ��
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;//�����б�ָ��

	Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;//������ָ��
	static const int SwapChainBufferCount = 2;//����������������
	int mCurrentBackBuffer = 0;//��ǰ�󻺳������
	Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];//������������ָ��
	Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;//���ģ�建����ָ��

	UINT mRTVDescriptorSize = 0;//RTV��������С
	UINT mDSVDescriptorSize = 0;//DSV��������С
	UINT mCBV_SRV_UAVDescriptorSize = 0;//CBV��SRV��UAV��������С
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRTVHeap;//RTV��������ָ��
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDSVHeap;//DSV��������ָ��

	D3D12_VIEWPORT mScreenViewport = {};//�ӿ�
	D3D12_RECT mScissorRect = {};//�ü�����

	//���±������������������ж���
	LPCTSTR mMainWndTitle=L"DefaultTitle";
	D3D_DRIVER_TYPE md3dDriverType= D3D_DRIVER_TYPE_HARDWARE;
	DXGI_FORMAT mBackBufferFormat= DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	int mClientWidth=800;
	int mClientHeight=600;
};
//�ࣺ�����������
class DXApp::WindowClass
{
public:
	WindowClass() = default;
	explicit WindowClass(HINSTANCE hInstance);
	~WindowClass();
public:
	const wchar_t* SetWCName(LPCTSTR WCName);//���ô���������
	const wchar_t* GetWCName()const;//���ش���������
	HINSTANCE GetInstance()const;//���ش�����ʵ�����
private:
	HINSTANCE hWndClassInst;
	const wchar_t* WindowClassName = nullptr;
};
//�ࣺ���ڵ�����
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
	const wchar_t* SetWndName(LPCTSTR WndName);//���ô�������
	void SetWndPos(int x, int y, int wx, int wy);//���ô�����������
	HWND GetWndHwnd()const;//��ȡ���ھ��
private:
	HWND WndHwnd = nullptr;
	LPCTSTR pWindowName = nullptr;
	int win_x = 0;
	int win_y = 0;
	int win_wx = 0;
	int win_wy = 0;
};