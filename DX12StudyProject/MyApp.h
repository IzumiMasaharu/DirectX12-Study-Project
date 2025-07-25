#include "DXApp.h"
#include "DDSTextureLoader.h"
#include "FrameResource.h"
#include "GeometryGenerator.h"
#include "RenderItem.h"
#include "UploadBuffer.h"

class MyApp : public DXApp
{
public:
	explicit MyApp(HINSTANCE hInstace);
	~MyApp() = default;
public:
	LRESULT MessageProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	bool Init() override;
private:
	void Resize() override;
	void Update(const GameTimer& GTimer) override;
	void Draw(const GameTimer& GTimer) override;

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
	void BuildImportedGeometry(); // 创建通过文件导入的模型
	void BuildMaterials(); // 创建材质
	void BuildRenderItems(); // 创建渲染项
	void BuildFrameResources(); // 创建帧资源
	void BuildPSOs(); // 创建渲染管线状态对象

	void ChangeW_H(int width, int height);
	void ChangePSOstate();
	void UpdateCamera(); // 更新摄像头矩阵
	void UpdateObjectsConstBuffers()const; // 更新常量缓冲区（世界矩阵）
	void UpdatePassConstBuffers()const; // 更新渲染过程常量缓冲区
	void UpdateMaterialConstBuffers()const; // 更新材质常量缓冲区

	void DrawRenderItems(ID3D12GraphicsCommandList* commandList, const std::vector<RenderItem*>& renderItems)const; // 绘制渲染项
public:
	const MyApp* GetMyApp()const; // 获取指向MyApp类自身的指针
private:
	WindowClass windowClass;
	Window appMainWnd;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr; // 根签名

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap = nullptr; // SRV描述符堆

	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout; // 输入布局
	UINT passCbvOffset = 0; // 渲染过程常量缓冲区偏移量

	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> shaders; // 储存着色器的无序图
	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> geos; // 储存几何网格体的无序图
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> PSOs; // 储存不同PSO的无序图
	std::unordered_map<std::string, std::unique_ptr<Material>> materials; // 存储材质的无序图
	std::unordered_map<std::string, std::unique_ptr<Texture>> textures; // 存储纹理的无序图
	
	std::vector<std::unique_ptr<RenderItem>> allRenderItems; // 储存有所有渲染项
	std::vector<RenderItem*> opaqueRenderItems; // 储存不透明渲染项
	std::vector<RenderItem*> transparentRenderItems; // 储存透明渲染项

	UINT currentFrameResourceIndex = 0; // 当前帧资源索引
	std::vector<std::unique_ptr<FrameResource>> frameResources; // 全部帧资源
	FrameResource* currentFrameResource = nullptr; // 当前帧资源

	POINT lastMousePosition;

	bool isWireframeEnabled = false;

	DirectX::XMFLOAT3 eyePosition = { 0.0f,0.0f,0.0f };
	float theta = 0; // 极点-原点在x-z面上投影与x轴正半轴夹角
	float phi = DirectX::XM_PIDIV4; // 极点-原点连线与Y轴正半轴夹角
	float radius = 15.0f; // 极径长

	DirectX::XMFLOAT4X4 viewTransform = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 projectionTransform = MathHelper::Identity4x4();
};

