#include "DXApp.h"
#include "DDSTextureLoader.h"
#include "FrameResource.h"
#include "GeometryGenerator.h"
#include "RenderItem.h"
#include "UploadBuffer.h"
#include "Camera.h"

class Render : public DXApp
{
public:
	explicit Render(HINSTANCE hInstace);
	~Render();
public:
	LRESULT MessageProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	bool Init() override;
private:
	void RenderLoop() override;                 // 渲染线程主循环

	void Resize() override;
	void Update(const GameTimer& GTimer) override;
	void Draw(const GameTimer& GTimer) override;

	void KeyboardMsgProc(UINT msg, WPARAM wParam, LPARAM lParam) override;
	void MouseDown(WPARAM ButtonState, int x, int y) override;
	void MouseUp(WPARAM ButtonState, int x, int y) override;
	void MouseMove(WPARAM ButtonState, int x, int y) override;
	void MouseWheel(short zDelta) override;
	 
	void LoadTexture(); // 载入纹理
	void BuildRootSignature(); // 创建根签名
	void BuildDescriptorHeaps(); // 创建程序所需的其他描述符堆（除初始化时创建的DSV、RTV描述符堆）
	void BuildShaders(); // 编译着色器
	void BuildInputLayout(); // 创建输入布局
	void BuildMeshGeometry(); // 创建网格体
	void BuildImportedGeometryFromOBJ(); // 创建通过文件导入的模型
	void BuildMaterials(); // 创建材质
	void BuildMaterialStructuredBuffers();	// 创建材质结构化缓冲区
	void BuildRenderItems(); // 创建渲染项
	void BuildFrameResources(); // 创建帧资源
	void BuildPSOs(); // 创建渲染管线状态对象

	void UpdateCameraState(const GameTimer& GTimer); // 移动摄像机

	void UpdateObjectsConstBuffers();	// 更新常量缓冲区（世界矩阵）
	void UpdatePassConstBuffers();		// 更新渲染过程常量缓冲区

	void DrawRenderItems(ID3D12GraphicsCommandList* commandList, const std::vector<RenderItem*>& renderItems)const; // 绘制渲染项
public:
	const Render* GetMyApp()const; // 获取指向MyApp类自身的指针
private:
	WindowClass windowClass;
	Window appMainWnd;

	struct ResizeInfoForRenderThread {
		std::atomic<bool> isResized{ false };
		std::atomic<int>  newWidth{ 0 };
		std::atomic<int>  newHeight{ 0 };
	};
	ResizeInfoForRenderThread resizeInfo;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;	// 根签名

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap = nullptr;	// SRV描述符堆

	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout; // 输入布局
	UINT passCbvOffset = 0;		// 渲染过程常量缓冲区偏移量

	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> shaders;			// 储存着色器的无序图
	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> geos;				// 储存几何网格体的无序图
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> PSOs;	// 储存不同PSO的无序图
	std::unordered_map<std::string, std::unique_ptr<Material>> materials;				// 存储材质的无序图
	std::unordered_map<std::string, std::unique_ptr<Texture>> textures;			// 存储纹理的无序图

	std::vector<RenderItem*> allRenderItems;							// 储存有所有渲染项
	std::vector<std::unique_ptr<RenderItem>> opaqueRenderItems;			// 储存不透明渲染项
	std::vector<std::unique_ptr<RenderItem>> transparentRenderItems;	// 储存透明渲染项

	std::vector<std::unique_ptr<FrameResource>> frameResources; // 全部帧资源
	UINT currentFrameResourceIndex = 0;							// 当前帧资源索引
	FrameResource* currentFrameResource = nullptr;				// 当前帧资源

	POINT lastMousePosition;

	bool isWireframeEnabled = false;

	Camera camera;	//摄像机对象
	bool isMoving = false;
	DirectX::XMFLOAT3 moveDirection{0.0f, 0.0f, 0.0f};
	float moveSpeed = 3.0f;

	bool isRolling = false;
	float rollingDirection = 0.0f;
	float rollingSpeed = 0.2f;
};