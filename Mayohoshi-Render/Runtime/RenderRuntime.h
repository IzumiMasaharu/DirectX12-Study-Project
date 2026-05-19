#pragma once

#include "D3D12RenderHost.h"

#include "MotionParam.h"
#include "FrameResource.h"
#include "Camera.h"
#include "RenderResourceManager.h"
#include "RenderSceneManager.h"
#include "DescriptorHeapManager.h"
#include "MayohoshiTypes.h"

#include <functional>
#include <mutex>
#include <vector>

class RenderRuntime : public D3D12RenderHost
{
public:
	explicit RenderRuntime(HINSTANCE hInstace);
	~RenderRuntime() final;

	LRESULT wndMsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	bool Initialize() override;
	void ConfigureOwnedWindow(UINT width, UINT height, const std::wstring& title);
	void UseExternalWindow(HWND hwnd, UINT width, UINT height);
	void SetStartThreads(bool enabled);
	void SetShaderDirectory(const std::wstring& directory);
	void ResizeRenderTarget(UINT width, UINT height);
	void RunResourceCommand(const std::function<void()>& command);
	void Stop();

	using SceneSetupCallback = std::function<void(RenderRuntime&)>;
	void SetSceneSetup(SceneSetupCallback callback);

	Mayohoshi::CanvasHandle CreateCanvas(const Mayohoshi::CanvasDesc& desc);
	Mayohoshi::TextureHandle CreateTexture(const Mayohoshi::TextureDesc& desc);
	Mayohoshi::TextureSetHandle BindTextures(const Mayohoshi::TextureSetDesc& desc);
	Mayohoshi::MaterialHandle CreateMaterial(const Mayohoshi::MaterialDesc& desc);
	Mayohoshi::ModelHandle CreateCylinder(const Mayohoshi::CylinderDesc& desc);
	Mayohoshi::ModelHandle CreateSphere(const Mayohoshi::SphereDesc& desc);
	Mayohoshi::ModelHandle CreateGrid(const Mayohoshi::GridDesc& desc);
	Mayohoshi::ModelHandle CreateModel(const Mayohoshi::ObjModelDesc& desc);
	Mayohoshi::RenderObjectHandle AddInstance(const Mayohoshi::DrawDesc& desc);
	Mayohoshi::RenderObjectHandle AddInstances(const Mayohoshi::InstanceBatchDesc& desc);
	Mayohoshi::RenderObjectHandle AddToRender(const Mayohoshi::DrawDesc& desc);
	void DrawTo(const Mayohoshi::CanvasHandle& canvas);
	void DrawToScreen(const Mayohoshi::CanvasHandle& canvas);
	void SetAmbientLight(const DirectX::XMFLOAT3& color, float intensity = 1.0f);
	Mayohoshi::LightHandle AddLight(const Mayohoshi::LightDesc& desc);
	bool SetLight(const Mayohoshi::LightHandle& light, const Mayohoshi::LightDesc& desc);
	bool SetLightPosition(const Mayohoshi::LightHandle& light, const DirectX::XMFLOAT3& position);
	bool SetLightDirection(const Mayohoshi::LightHandle& light, const DirectX::XMFLOAT3& direction);
	bool SetLightIntensity(const Mayohoshi::LightHandle& light, const DirectX::XMFLOAT3& intensity);
	void ClearLights();
	void SetUpdateCallback(Mayohoshi::UpdateCallback callback);
	bool SetObjectTransform(const Mayohoshi::RenderObjectHandle& object, const Mayohoshi::TransformDesc& transform, size_t instanceIndex = 0);
	void SetCameraPosition(const DirectX::XMFLOAT3& position);
	void LookAt(const DirectX::XMFLOAT3& target);
	void SetWireframe(bool enabled);
private:
	void RenderLoop() override;						// 渲染线程主循环

	void Resize() override;
	void Update(const GameTimer& GTimer) override;
	void Draw(const GameTimer& GTimer) override;

	void KeyboardMsgProc(UINT vk, bool pressed) override;
	void MouseDown(WPARAM ButtonState, int x, int y) override;
	void MouseUp(WPARAM ButtonState, int x, int y) override;
	void MouseMove(WPARAM ButtonState, int x, int y) override;

	void LoadTexture();								// 载入纹理
	void BuildRootSignature();						// 创建根签名
	void BuildDescriptorHeaps();					// 创建程序所需的其他描述符堆（除初始化时创建的DSV、RTV描述符堆）
	void BuildShaders();							// 编译着色器
	void BuildInputLayout();						// 创建输入布局
	void BuildMeshGeometry();						// 创建网格体
	void BuildImportedGeometryFromOBJ();			// 创建通过文件导入的模型
	void BuildMaterials();							// 创建材质
	void BuildRenderItems();						// 创建渲染项
	void buildInstances();							// 创建实例数据	
	void BuildFrameResources();						// 创建帧资源
	void BuildPSOs();								// 创建渲染管线状态对象

	void UpdateCameraState(const GameTimer& GTimer);// 移动摄像机

	void updateMaterialBuffers();					// 创建材质结构化缓冲区
	void updateTextureTableBuffers();				// 更新纹理表结构化缓冲区
	void updateInstanceDataBuffers();				// 更新常量缓冲区（世界矩阵）
	void updatePassConstBuffers();					// 更新渲染过程常量缓冲区

	void DrawRenderItems(ID3D12GraphicsCommandList* commandList, const std::vector<RenderItem*>& renderItems)const; // 绘制渲染项
public:
	const RenderRuntime* getAppPtr()const;					// 获取指向MyApp类自身的指针
private:
	struct ResizeInfoForRenderThread {
		std::atomic<bool> isResized{ false };
		std::atomic<int>  newWidth{ 0 };
		std::atomic<int>  newHeight{ 0 };
	};
	ResizeInfoForRenderThread resizeInfo;

	SceneSetupCallback sceneSetupCallback;
	bool useExternalWindow = false;
	bool startThreads = true;
	bool resourceBindingReady = false;
	std::wstring ownedWindowTitle;
	std::wstring shaderDirectory = L"../Shaders/";

	RenderResourceManager renderSourceManager;
	RenderSceneManager sceneManager;
	mutable std::mutex sceneMutex;
	mutable std::mutex resourceMutex;
	mutable std::mutex lightingMutex;
	DirectX::XMFLOAT4 ambientLight = { 0.2f,0.2f,0.2f,1.0f };
	std::vector<Light> directionalLights;
	std::vector<Light> pointLights;
	std::vector<Light> spotLights;
	std::mutex updateCallbackMutex;
	Mayohoshi::UpdateCallback updateCallback;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;	// 根签名

	SrvDescriptorHeap srvDescriptorHeap;	// SRV描述符堆

	std::unordered_map<std::string, std::vector<D3D12_INPUT_ELEMENT_DESC> > inputLayout; // 输入布局
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> shaders;			// 储存着色器的无序图 
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> PSOs;	// 储存不同PSO的无序图

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> meshes;				// 储存几何网格体的无序图
	std::unordered_map<std::string, std::unique_ptr<Material>> materials;				// 存储材质的无序图
	std::unordered_map<std::string, std::unique_ptr<Texture>> textures;					// 存储纹理的无序图

	std::vector<RenderItem*> allRenderItems;							// 储存有所有渲染项
	std::unordered_map<std::string, uint32_t> renderItemIndex;			// 储存渲染项的无序图，便于查找
	std::vector<std::unique_ptr<RenderItem>> opaqueRenderItems;			// 储存不透明渲染项
	std::vector<std::unique_ptr<RenderItem>> skycubeRenderItem;			// 天空盒渲染项
	std::vector<std::unique_ptr<RenderItem>> transparentRenderItems;	// 储存透明渲染项

	std::vector<std::unique_ptr<FrameResource>> frameResources; // 全部帧资源
	UINT currentFrameResourceIndex = 0;							// 当前帧资源索引
	FrameResource* currentFrameResource = nullptr;				// 当前帧资源

	POINT lastMousePosition = {0,0};

	bool isWireframeEnabled = false;

	//摄像机对象
	Camera camera;
	MotionParam cameraMotionParam;

	std::array<bool, 256> keyDown = {};
};
