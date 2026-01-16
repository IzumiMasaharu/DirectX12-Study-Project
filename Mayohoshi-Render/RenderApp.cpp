#include "RenderApp.h"

#include "resource.h"
#include "DDSTextureLoader.h"
#include "GeometryGenerator.h"

using namespace DirectX;
using namespace Microsoft::WRL;

RenderApp::RenderApp(HINSTANCE hInstance) : DxApp(hInstance)
{
	wndClassDesc.className = L"Mayohoshi Render WndClass";
	wndClassDesc.style = CS_HREDRAW | CS_VREDRAW;
	wndClassDesc.icon = static_cast<HICON>(LoadImage(appInstance, MAKEINTRESOURCE(RENDER), IMAGE_ICON, 512, 512, LR_VGACOLOR));
	
	wndDesc.title = L"Mayohoshi Render";
	wndDesc.clientWidth = 1280;
	wndDesc.clientHeight = 720;

	d3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
	backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	clientHeight = wndDesc.clientHeight;
	clientWidth = wndDesc.clientWidth;
}
RenderApp::~RenderApp()
{
	isAppRunning = false;
	isAppPaused = true;
	isRenderThreadRunning = false;
	isRenderPaused = true;

	if (renderThread.joinable())
		renderThread.join();
	if (controlThread.joinable())
		controlThread.join();

	if (d3dDevice != nullptr)
		FlushCommandQueue();
}

// 消息过程处理函数
LRESULT RenderApp::wndMsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_ACTIVATE:
		if (HIWORD(wParam) == 0) // 激活渲染线程
			isRenderPaused = false;
		else
			isRenderPaused = true;
		return 0;
	case WM_SIZE:
	{
		if (wParam == SIZE_MINIMIZED)
		{
			isRenderPaused = true;
		}
		else
		{
			isRenderPaused = false;
			resizeInfo.newWidth = LOWORD(lParam);
			resizeInfo.newHeight = HIWORD(lParam);
			resizeInfo.isResized = true;
		}
		return 0;
	}
	case WM_CLOSE:
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_LBUTTONDOWN:
		MouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		return 0;
	case WM_LBUTTONUP:
		MouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		return 0;
	case WM_MOUSEMOVE:
		MouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	// case WM_MOUSEWHEEL:
	//    return 0;
	case WM_INPUT:
	{
	    UINT dwSize = 0;
	    GetRawInputData((HRAWINPUT)lParam,RID_INPUT,nullptr,&dwSize,sizeof(RAWINPUTHEADER));

	    if (dwSize == 0)
	        return 0;

	    std::vector<BYTE> buffer(dwSize);
	    if (GetRawInputData((HRAWINPUT)lParam,RID_INPUT,buffer.data(),&dwSize,sizeof(RAWINPUTHEADER)) != dwSize)
	        return 0;

	    const RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(buffer.data());
	    if (raw->header.dwType != RIM_TYPEKEYBOARD)
	        return 0;

	    const RAWKEYBOARD& kb = raw->data.keyboard;

	    bool pressed = !(kb.Flags & RI_KEY_BREAK);

	    KeyboardMsgProc(kb.VKey, pressed);
	    return 0;
	}

	default:
		return DxApp::wndMsgProc(hwnd, msg, wParam, lParam);
	}
}

// 应用程序初始化
bool RenderApp::init()
{
	if (!windowsManager.initWindowClass(wndClassDesc))
		return false;
	if (!windowsManager.initWindow(L"MainWindow",wndDesc))
		return false;
	else
		mainWndHwnd = windowsManager.getHwnd(L"MainWindow");
	if (!DxApp::InitDirectX3D())
		return false;
	Resize();

	ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr));
	LoadTexture();
	BuildRootSignature();
	BuildDescriptorHeaps();
	BuildShaders();
	BuildInputLayout();
	BuildMeshGeometry();
	BuildImportedGeometryFromOBJ();
	BuildMaterials();
	BuildRenderItems();
	BuildFrameResources();
	BuildPSOs();

	ThrowIfFailed(commandList->Close());
	std::array<ID3D12CommandList*, 1> cmdsLists = { commandList.Get() };
	commandQueue->ExecuteCommandLists(static_cast<UINT>(cmdsLists.size()), cmdsLists.data());
	FlushCommandQueue();

	isAppRunning = true;
	isAppPaused = false;
	isRenderThreadRunning = true;
	isRenderPaused = false;

	gameTimer.Reset();

	controlThread = std::thread(&RenderApp::ControlLoop, this);
	renderThread = std::thread(&RenderApp::RenderLoop, this);

	return true;
}

// 渲染线程循环
void RenderApp::RenderLoop()
{
	while (isRenderThreadRunning)
	{
		// 等待主线程资源更新完成
		std::unique_lock<std::mutex> lock(renderMutex);
		renderCV.wait(lock, [&] { return isFrameReady.load(); });

		// 渲染和呈现
		if (!isRenderPaused)
		{
			if (resizeInfo.isResized.load(std::memory_order_relaxed))
			{
				clientWidth = resizeInfo.newWidth.load();
				clientHeight = resizeInfo.newHeight.load();
				// 防止 0 尺寸 避免除0问题
				if (clientWidth > 0 && clientHeight > 0)
					Resize();

				resizeInfo.isResized = false;
			}

			Draw(gameTimer);

			CalculateFPS_MSPF();
		}

		// 标记本帧渲染完成
		isFrameRendered = true;
		isFrameReady = false;
	}
}

// 窗口大小重新适配
void RenderApp::Resize()
{
	DxApp::Resize();

	// 设置视口
	screenViewport.Height = static_cast<float>(clientHeight);
	screenViewport.Width = static_cast<float>(clientWidth);
	screenViewport.TopLeftX = 0;
	screenViewport.TopLeftY = 0;
	screenViewport.MaxDepth = 1.0f;
	screenViewport.MinDepth = 0.0f;
	scissorRect = { 0,0,clientWidth,clientHeight };

	camera.setAspectRatio(W_H_Ratio());
}

// 更新帧画面
void RenderApp::Update(const GameTimer& GTimer)
{
	UpdateCameraState(GTimer);
	currentFrameResourceIndex = (currentFrameResourceIndex + 1) % gNumFrameResources;
	currentFrameResource = frameResources[currentFrameResourceIndex].get();

	if (currentFrameResource->fence != 0 && fence->GetCompletedValue() < currentFrameResource->fence)
	{
		HANDLE event = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(fence->SetEventOnCompletion(currentFrameResource->fence, event));
		if (event)
		{
			WaitForSingleObject(event, INFINITE);
			CloseHandle(event);
		}
	}

	updateInstanceDataBuffers();
	updateMaterialBuffers();
	updatePassConstBuffers();

	std::ostringstream os;
	os << "Mayohoshi Render FPS: " << fps;
	SetWindowText(mainWndHwnd, AnsiToWstring(os.str()).c_str());
}

// 绘制帧画面
void RenderApp::Draw(const GameTimer& GTimer)
{
	auto cmdListAllocator = currentFrameResource->commandAllocator;

	ThrowIfFailed(cmdListAllocator->Reset());
	if (isWireframeEnabled)
		commandList->Reset(cmdListAllocator.Get(), PSOs["Wireframe"].Get());
	else
		commandList->Reset(cmdListAllocator.Get(), PSOs["Solid"].Get());

	commandList->RSSetViewports(1, &screenViewport);
	commandList->RSSetScissorRects(1, &scissorRect);

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	const std::array<FLOAT, 4> clearScreenColor = { 0.0f, 0.0f, 0.0f, 1.0f };
	commandList->ClearRenderTargetView(CurrentBackBufferView(), clearScreenColor.data(), 0, nullptr); 
	commandList->ClearDepthStencilView(DepthStencilBufferView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	commandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilBufferView());

	std::array<ID3D12DescriptorHeap*, 1> descriptorHeaps = { srvDescriptorHeap.Get() };
	commandList->SetDescriptorHeaps(static_cast<UINT>(descriptorHeaps.size()), descriptorHeaps.data());

	commandList->SetGraphicsRootSignature(rootSignature.Get());

	auto passConstBuffer = currentFrameResource->passConstBuffer->Resource();
	commandList->SetGraphicsRootConstantBufferView(3, passConstBuffer->GetGPUVirtualAddress());
	auto materialStructedBuffer = currentFrameResource->materialStructuredBuffer->Resource();
	commandList->SetGraphicsRootShaderResourceView(0, materialStructedBuffer->GetGPUVirtualAddress());

	CD3DX12_GPU_DESCRIPTOR_HANDLE texDescriptor(srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	commandList->SetGraphicsRootDescriptorTable(4, texDescriptor);
	texDescriptor.Offset(textures["skycube"].get()->srvHeapIndex, cbs_srv_uavDescriptorSize);
	commandList->SetGraphicsRootDescriptorTable(5, texDescriptor);

	DrawRenderItems(commandList.Get(), opaqueRenderItems);
	commandList->SetPipelineState(PSOs["SkyCube"].Get());
	DrawRenderItems(commandList.Get(), skycubeRenderItem);
	commandList->SetPipelineState(PSOs["Wave"].Get());
	DrawRenderItems(commandList.Get(), transparentRenderItems);

	commandList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(commandList->Close());

	std::array<ID3D12CommandList*, 1> CommandList = { commandList.Get() };
	commandQueue->ExecuteCommandLists(static_cast<UINT>(CommandList.size()), CommandList.data());

	ThrowIfFailed(swapChain->Present(0, 0));

	currentBackBuffer = (currentBackBuffer + 1) % SwapChainBufferCount;
	currentFrameResource->fence = ++currentFenceValue;
	commandQueue->Signal(fence.Get(), currentFenceValue);
}

// 处理键盘输入
void RenderApp::KeyboardMsgProc(UINT vk, bool pressed)
{
	keyDown[vk] = pressed;

	if (vk == VK_ESCAPE && pressed)
		PostQuitMessage(0);

	if (vk == '1' && pressed)
		isWireframeEnabled = !isWireframeEnabled;
}

// 当鼠标按下时调用
void RenderApp::MouseDown(WPARAM ButtonState, int x, int y)
{
	lastMousePosition.x = x;
	lastMousePosition.y = y;

	SetCapture(mainWndHwnd);
}
// 当鼠标抬起时调用
void RenderApp::MouseUp(WPARAM ButtonState, int x, int y)
{
	ReleaseCapture();
}
// 当鼠标移动时调用
void RenderApp::MouseMove(WPARAM ButtonState, int x, int y)
{
	if ((ButtonState & MK_LBUTTON) != 0)
	{
		float pitch = static_cast<float>(y - lastMousePosition.y) * 0.002f;
		float yaw = static_cast<float>(x - lastMousePosition.x) * 0.002f;

		camera.rotate(0.0f,pitch,yaw);
	}

	lastMousePosition.x = x;
	lastMousePosition.y = y;
}

// 载入纹理
void RenderApp::LoadTexture()
{
	UINT srvIndex = 0;

	std::vector<std::string> staticTexNames =
	{
		"stone",
		"brick",
		"wave",
		"floor",
		"wood",

		"defaultNormal",
		"brickNormal",
		"waveNormal",
		"floorNormal",

		"defaultDepth",
		"brickDepth",
		"floorDepth",
	};
	std::vector<std::wstring> staticTexFilepathes =
	{
		L"../Resources/Textures/stone.dds",
		L"../Resources/Textures/bricks.dds",
		L"../Resources/Textures/water.dds",
		L"../Resources/Textures/floor.dds",
		L"../Resources/Textures/wood.dds",

		L"../Resources/Textures/default_normal.dds",
		L"../Resources/Textures/brick_normal.dds",
		L"../Resources/Textures/wave.dds",
		L"../Resources/Textures/floor_normal.dds",

		L"../Resources/Textures/default_depth.dds",
		L"../Resources/Textures/brick_depth.dds",
		L"../Resources/Textures/floor_depth.dds",
	};

	std::vector<std::string> cubeTexNames =
	{
		"skycube",
	};
	std::vector<std::wstring> cubeTexFilepathes =
	{
		L"../Resources/Textures/snow_skycube.dds"
	};

	for (int i = 0; i < (int)staticTexNames.size(); ++i)
	{
		auto tex = std::make_unique<Texture>();
		tex->name = staticTexNames[i];
		tex->srvHeapIndex = srvIndex++;

		ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(
			d3dDevice.Get(), commandList.Get(), staticTexFilepathes[i].c_str(), tex->resource, tex->uploadHeap));

		textures[tex->name] = std::move(tex);
	}

	for (int i = 0; i < (int)cubeTexNames.size(); ++i)
	{
		auto tex = std::make_unique<Texture>();
		tex->name = cubeTexNames[i];
		tex->srvHeapIndex = srvIndex++;

		ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(
			d3dDevice.Get(), commandList.Get(), cubeTexFilepathes[i].c_str(), tex->resource, tex->uploadHeap));

		textures[tex->name] = std::move(tex);
	}
}
// 创建根签名
void RenderApp::BuildRootSignature()
{
	// 描述符范围，一段连续、类型相同的描述符
	CD3DX12_DESCRIPTOR_RANGE textureSrvRange;
	textureSrvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 16, 0, 0);
	CD3DX12_DESCRIPTOR_RANGE skycubeSrvRange;
	skycubeSrvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 2);

	const UINT numSlotRootParams = 6;
	std::array<CD3DX12_ROOT_PARAMETER, numSlotRootParams> slotRootParameter;
	slotRootParameter[0].InitAsShaderResourceView(0, 1);											// 绑定结构化材质数据缓冲区
	slotRootParameter[1].InitAsShaderResourceView(1, 1);											// 绑定结构化贴图表数据缓冲区
	slotRootParameter[2].InitAsShaderResourceView(2, 1);											// 绑定结构化实例数据缓冲区
	slotRootParameter[3].InitAsConstantBufferView(0);												// 绑定渲染常量缓冲区
	slotRootParameter[4].InitAsDescriptorTable(1, &textureSrvRange, D3D12_SHADER_VISIBILITY_PIXEL);	// 描述符表 存储一系列描述符范围
	slotRootParameter[5].InitAsDescriptorTable(1, &skycubeSrvRange, D3D12_SHADER_VISIBILITY_PIXEL);

	auto staticSamplers = DxUtil::GetStaticSamplers();

	CD3DX12_ROOT_SIGNATURE_DESC RootSignatureDesc(
		numSlotRootParams,
		slotRootParameter.data(),
		(UINT)staticSamplers.size(),
		staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	ComPtr<ID3DBlob> serializedRootSignature = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSignature.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	ThrowIfFailed(hr);

	ThrowIfFailed(d3dDevice->CreateRootSignature(0, serializedRootSignature->GetBufferPointer(),
		serializedRootSignature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
}
// 创建程序所需的其他描述符堆（除初始化时创建的DSV、RTV描述符堆）
void RenderApp::BuildDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC SRV_HEAP_DESC;
	SRV_HEAP_DESC.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	SRV_HEAP_DESC.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	SRV_HEAP_DESC.NumDescriptors = (UINT)textures.size();
	SRV_HEAP_DESC.NodeMask = 0;
	ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&SRV_HEAP_DESC, IID_PPV_ARGS(&srvDescriptorHeap)));
	
	for (const auto& tex : textures)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE srvCPUHandle(srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

		D3D12_RESOURCE_DESC desc = tex.second->resource->GetDesc();
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = desc.Format;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		switch (desc.Dimension)
		{
		case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
			srvDesc.ViewDimension = (desc.DepthOrArraySize > 1) ? D3D12_SRV_DIMENSION_TEXTURE1DARRAY : D3D12_SRV_DIMENSION_TEXTURE1D;
			if (srvDesc.ViewDimension == D3D12_SRV_DIMENSION_TEXTURE1D)
			{
				srvDesc.Texture1D.MostDetailedMip = 0;
				srvDesc.Texture1D.MipLevels = desc.MipLevels;
				srvDesc.Texture1D.ResourceMinLODClamp = 0.0f;
			}
			else
			{
				srvDesc.Texture1DArray.MostDetailedMip = 0;
				srvDesc.Texture1DArray.MipLevels = desc.MipLevels;
				srvDesc.Texture1DArray.FirstArraySlice = 0;
				srvDesc.Texture1DArray.ArraySize = desc.DepthOrArraySize;
				srvDesc.Texture1DArray.ResourceMinLODClamp = 0.0f;
			}
			break;

		case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
			if (desc.DepthOrArraySize == 6)
			{
				// Cube
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
				srvDesc.TextureCube.MostDetailedMip = 0;
				srvDesc.TextureCube.MipLevels = desc.MipLevels;
				srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
			}
			else if (desc.DepthOrArraySize > 6 && desc.DepthOrArraySize % 6 == 0)
			{
				// Cube Array
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
				srvDesc.TextureCubeArray.MostDetailedMip = 0;
				srvDesc.TextureCubeArray.MipLevels = desc.MipLevels;
				srvDesc.TextureCubeArray.First2DArrayFace = 0;
				srvDesc.TextureCubeArray.NumCubes = desc.DepthOrArraySize / 6;
				srvDesc.TextureCubeArray.ResourceMinLODClamp = 0.0f;
			}
			else if (desc.SampleDesc.Count > 1)
			{
				srvDesc.ViewDimension = (desc.DepthOrArraySize > 1) ?
					D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY : D3D12_SRV_DIMENSION_TEXTURE2DMS;
				if (srvDesc.ViewDimension == D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY)
				{
					srvDesc.Texture2DMSArray.FirstArraySlice = 0;
					srvDesc.Texture2DMSArray.ArraySize = desc.DepthOrArraySize;
				}
			}
			else if (desc.DepthOrArraySize > 1)
			{
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
				srvDesc.Texture2DArray.MostDetailedMip = 0;
				srvDesc.Texture2DArray.MipLevels = desc.MipLevels;
				srvDesc.Texture2DArray.FirstArraySlice = 0;
				srvDesc.Texture2DArray.ArraySize = desc.DepthOrArraySize;
				srvDesc.Texture2DArray.PlaneSlice = 0;
				srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
			}
			else
			{
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				srvDesc.Texture2D.MostDetailedMip = 0;
				srvDesc.Texture2D.MipLevels = desc.MipLevels;
				srvDesc.Texture2D.PlaneSlice = 0;
				srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
			}
			break;

		case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
			srvDesc.Texture3D.MostDetailedMip = 0;
			srvDesc.Texture3D.MipLevels = desc.MipLevels;
			srvDesc.Texture3D.ResourceMinLODClamp = 0.0f;
			break;

		default:
			assert(false && "Unsupported texture dimension!");
			continue;
		}

		srvCPUHandle.Offset(tex.second->srvHeapIndex, cbs_srv_uavDescriptorSize);
		d3dDevice->CreateShaderResourceView(tex.second->resource.Get(), &srvDesc, srvCPUHandle);
	}
}
// 编译着色器
void RenderApp::BuildShaders()
{
	shaders["VS"] = DxUtil::CompileShaderOnline(L"..\\Shaders\\Vertex_Common.hlsl", nullptr, "VS", "vs_5_1");
	shaders["PS"] = DxUtil::CompileShaderOnline(L"..\\Shaders\\Fragment.hlsl", nullptr, "PS", "ps_5_1");
	shaders["VS_Wave"] = DxUtil::CompileShaderOnline(L"..\\Shaders\\Vertex_AniWave.hlsl", nullptr, "WaveVS", "vs_5_1");
	shaders["VS_Skycube"] = DxUtil::CompileShaderOnline(L"..\\Shaders\\Skycube.hlsl", nullptr, "SkycubeVS", "vs_5_1");
	shaders["PS_Skycube"] = DxUtil::CompileShaderOnline(L"..\\Shaders\\Skycube.hlsl", nullptr, "SkycubePS", "ps_5_1");
}
// 创建输入布局
void RenderApp::BuildInputLayout()
{
	inputLayout["Default"] =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TANGENT",0,DXGI_FORMAT_R32G32B32_FLOAT,0,24,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	};
	inputLayout["Skycube"] =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},		
		{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	};
}
// 创建网格体
void RenderApp::BuildMeshGeometry()
{
	GeometryGenerator::MeshData cylinder = GeometryGenerator::CreateCylinder(1.0f, 0.56f, 4.0f, 100, 20);
	GeometryGenerator::MeshData ball = GeometryGenerator::CreateBall(1.0f, 50, 50);
	GeometryGenerator::MeshData gird = GeometryGenerator::CreateGird(10.0f, 10.0f, 100, 100);

	UINT CylinderVertexOffset = 0;
	auto BallVertexOffset = (UINT)cylinder.vertices.size();
	auto GridVertexOffset = (UINT)(cylinder.vertices.size() + ball.vertices.size());

	UINT CylinderIndexOffset = 0;
	auto BallIndexOffset = (UINT)cylinder.indices32.size();
	auto GridIndexOffset = (UINT)(cylinder.indices32.size() + ball.indices32.size());

	SubmeshGeometry Geo_Cylinder;
	Geo_Cylinder.name = "Geo_Cylinder";
	Geo_Cylinder.vertexBaseLocation = CylinderVertexOffset;
	Geo_Cylinder.indexStartLocation = CylinderIndexOffset;
	Geo_Cylinder.indexCount = (UINT)cylinder.indices32.size();
	SubmeshGeometry Geo_Ball;
	Geo_Ball.name = "Geo_Ball";
	Geo_Ball.vertexBaseLocation = BallVertexOffset;
	Geo_Ball.indexStartLocation = BallIndexOffset;
	Geo_Ball.indexCount = (UINT)ball.indices32.size();
	SubmeshGeometry Geo_Gird;
	Geo_Gird.name = "Geo_Gird";
	Geo_Gird.vertexBaseLocation = GridVertexOffset;
	Geo_Gird.indexStartLocation = GridIndexOffset;
	Geo_Gird.indexCount = (UINT)gird.indices32.size();

	auto totalVertexCount = ball.vertices.size() + cylinder.vertices.size() + gird.vertices.size();

	std::vector<VertexConstants> vertices(totalVertexCount);
	std::vector<std::uint16_t> indices;
	UINT k = 0;
	for (size_t i = 0; i < cylinder.vertices.size(); ++i, ++k)
	{
		vertices[k].pos = cylinder.vertices[i].position;
		vertices[k].normal = cylinder.vertices[i].normal;
		vertices[k].tangent = cylinder.vertices[i].tangent;
		vertices[k].textureUV = cylinder.vertices[i].textureUV;
	}
	for (size_t i = 0; i < ball.vertices.size(); ++i, ++k)
	{
		vertices[k].pos = ball.vertices[i].position;
		vertices[k].normal = ball.vertices[i].normal;
		vertices[k].tangent = ball.vertices[i].tangent;
		vertices[k].textureUV = ball.vertices[i].textureUV;
	}
	for (size_t i = 0; i < gird.vertices.size(); ++i, ++k)
	{
		vertices[k].pos = gird.vertices[i].position;
		vertices[k].normal = gird.vertices[i].normal;
		vertices[k].tangent = gird.vertices[i].tangent;
		vertices[k].textureUV = gird.vertices[i].textureUV;
	}

	indices.insert(indices.end(), std::begin(cylinder.getIndices16()), std::end(cylinder.getIndices16()));
	indices.insert(indices.end(), std::begin(ball.getIndices16()), std::end(ball.getIndices16()));
	indices.insert(indices.end(), std::begin(gird.getIndices16()), std::end(gird.getIndices16()));

	const UINT vertexBufferByteSize = (UINT)vertices.size() * sizeof(VertexConstants);
	const UINT indexBufferByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto Geo = std::make_unique<MeshGeometry>();
	Geo->name = "Geo";

	ThrowIfFailed(D3DCreateBlob(vertexBufferByteSize, &Geo->vertexBufferCPU));
	CopyMemory(Geo->vertexBufferCPU->GetBufferPointer(), vertices.data(), vertexBufferByteSize);
	ThrowIfFailed(D3DCreateBlob(indexBufferByteSize, &Geo->indexBufferCPU));
	CopyMemory(Geo->indexBufferCPU->GetBufferPointer(), indices.data(), indexBufferByteSize);

	Geo->vertexBufferGPU = DxUtil::CreateDefaultBuffer(d3dDevice.Get(), commandList.Get(), vertices.data(), vertexBufferByteSize, Geo->vertexBufferUploader);
	Geo->indexBufferGPU = DxUtil::CreateDefaultBuffer(d3dDevice.Get(), commandList.Get(), indices.data(), indexBufferByteSize, Geo->indexBufferUploader);

	Geo->vertexByteStride = sizeof(VertexConstants);
	Geo->vertexBufferByteSize = vertexBufferByteSize;
	Geo->indexFormat = DXGI_FORMAT_R16_UINT;
	Geo->indexBufferByteSize = indexBufferByteSize;

	Geo->submeshList[Geo_Cylinder.name] = Geo_Cylinder;
	Geo->submeshList[Geo_Ball.name] = Geo_Ball;
	Geo->submeshList[Geo_Gird.name] = Geo_Gird;

	meshes[Geo->name] = std::move(Geo);
}
void RenderApp::BuildImportedGeometryFromOBJ()
{
	GeometryGenerator::MeshData nailong = GeometryGenerator::CreateImportedGeometryFromOBJ(L"../Resources/Models/Nailong.obj");

	UINT nailongVertexOffset = 0;
	UINT nailongIndexOffset = 0;

	auto totalVertexCount = nailong.vertices.size();

	std::vector<VertexConstants> vertices(totalVertexCount);
	std::vector<std::uint32_t> indices;
	UINT k = 0;
	for (size_t i = 0; i < nailong.vertices.size(); ++i, ++k)
	{
		vertices[k].pos = nailong.vertices[i].position;
		vertices[k].normal = nailong.vertices[i].normal;
		vertices[k].tangent = nailong.vertices[i].tangent;
		vertices[k].textureUV = nailong.vertices[i].textureUV;
	}

	indices.insert(indices.end(), std::begin(nailong.getIndices32()), std::end(nailong.getIndices32()));

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(VertexConstants);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint32_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->name = "objGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->vertexBufferCPU));
	CopyMemory(geo->vertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->indexBufferCPU));
	CopyMemory(geo->indexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->vertexBufferGPU = DxUtil::CreateDefaultBuffer(d3dDevice.Get(), commandList.Get(), vertices.data(), vbByteSize, geo->vertexBufferUploader);
	geo->indexBufferGPU = DxUtil::CreateDefaultBuffer(d3dDevice.Get(), commandList.Get(), indices.data(), ibByteSize, geo->indexBufferUploader);

	geo->vertexByteStride = sizeof(VertexConstants);
	geo->vertexBufferByteSize = vbByteSize;
	geo->indexFormat = DXGI_FORMAT_R32_UINT;
	geo->indexBufferByteSize = ibByteSize;

	SubmeshGeometry nailongSubMesh;
	nailongSubMesh.name = "Nailong";
	nailongSubMesh.vertexBaseLocation = nailongVertexOffset;
	nailongSubMesh.indexStartLocation = nailongIndexOffset;
	nailongSubMesh.indexCount = (UINT)indices.size();

	geo->submeshList[nailongSubMesh.name] = nailongSubMesh;
	meshes[geo->name] = std::move(geo);
}
// 创建材质
void RenderApp::BuildMaterials()
{
	UINT MaterialIndex = 0;

	auto matBrick = std::make_unique<Material>();
	matBrick->name = "Brick";
	matBrick->materialIndex = MaterialIndex++;
	matBrick->numDirtyFrames = gNumFrameResources;
	matBrick->albedo = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	matBrick->metallic = 0.0f;
	matBrick->roughness = 0.55f;
	matBrick->ior = 1.5f;
	auto matStone = std::make_unique<Material>();
	matStone->name = "Stone";
	matStone->materialIndex = MaterialIndex++;
	matStone->numDirtyFrames = gNumFrameResources;
	matStone->albedo = XMFLOAT4(0.9f, 0.9f, 0.9f, 1.0f);
	matStone->metallic = 0.0f;
	matStone->roughness = 0.15f;
	matStone->ior = 1.5f;
	auto matWood = std::make_unique<Material>();
	matWood->name = "Wood";
	matWood->materialIndex = MaterialIndex++;
	matWood->numDirtyFrames = gNumFrameResources;
	matWood->albedo = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	matWood->metallic = 0.0f;
	matWood->roughness = 0.4f;
	matWood->ior = 1.5f;
	auto matWater = std::make_unique<Material>();
	matWater->name = "Water";
	matWater->materialIndex = MaterialIndex++;
	matWater->numDirtyFrames = gNumFrameResources;
	matWater->albedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.6f);
	matWater->metallic = 0.0f;
	matWater->roughness = 0.02f;
	matWater->ior = 1.33f;
	auto matSkycube = std::make_unique<Material>();
	matSkycube->name = "Skycube";
	matSkycube->materialIndex = MaterialIndex++;
	matSkycube->numDirtyFrames = gNumFrameResources;
	matSkycube->albedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	matSkycube->metallic = 0.0f;
	matSkycube->roughness = 1.0f;
	matSkycube->ior = 1.0f;

	materials[matBrick->name] = std::move(matBrick);
	materials[matStone->name] = std::move(matStone);
	materials[matWood->name] = std::move(matWood);
	materials[matWater->name] = std::move(matWater);
	materials[matSkycube->name] = std::move(matSkycube);
}
// 创建结构化材质常量缓冲区
void RenderApp::updateMaterialBuffers()
{
	auto currentMaterialStructuredBuffer = currentFrameResource->materialStructuredBuffer.get();

	for (const auto& ma : materials)
	{
		Material* mat = ma.second.get();
		if (mat->numDirtyFrames > 0)
		{
			MaterialData materialData;
			materialData.albedo = mat->albedo;

			auto F0_dielectric_scalar = (float)pow((mat->ior - 1.0) / (mat->ior + 1.0), 2.0);
			auto F0_dielectric = DirectX::XMFLOAT3(F0_dielectric_scalar, F0_dielectric_scalar, F0_dielectric_scalar);
			auto F0_metal = DirectX::XMFLOAT3(mat->albedo.x, mat->albedo.y, mat->albedo.z);
			XMVECTOR vDielectric = XMLoadFloat3(&F0_dielectric);
			XMVECTOR vMetal = XMLoadFloat3(&F0_metal);
			XMVECTOR vResult = XMVectorLerp(vDielectric, vMetal, mat->metallic);
			XMStoreFloat3(&materialData.fresnel, vResult);

			materialData.roughness = mat->roughness;
			materialData.emissive = mat->emissive;
			materialData.materialTransform = mat->materialTransform;
			currentMaterialStructuredBuffer->CopyData(mat->materialIndex, materialData);

			mat->numDirtyFrames--;
		}
	}
}

// 创建渲染项
void RenderApp::BuildRenderItems()
{
	allRenderItems.clear();
	opaqueRenderItems.clear();
	skycubeRenderItem.clear();
	transparentRenderItems.clear();

	// 不透明渲染项
	{
		auto cylinderRenderItem = std::make_unique<RenderItem>();
		cylinderRenderItem->Geo = meshes["Geo"].get();
		cylinderRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		cylinderRenderItem->indexCount = cylinderRenderItem->Geo->submeshList["Geo_Cylinder"].indexCount;
		cylinderRenderItem->indexStartLocation = cylinderRenderItem->Geo->submeshList["Geo_Cylinder"].indexStartLocation;
		cylinderRenderItem->vertexBaseLocation = cylinderRenderItem->Geo->submeshList["Geo_Cylinder"].vertexBaseLocation;

		auto ballRenderItem = std::make_unique<RenderItem>();
		ballRenderItem->Geo = meshes["Geo"].get();
		ballRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		ballRenderItem->indexCount = ballRenderItem->Geo->submeshList["Geo_Ball"].indexCount;
		ballRenderItem->indexStartLocation = ballRenderItem->Geo->submeshList["Geo_Ball"].indexStartLocation;
		ballRenderItem->vertexBaseLocation = ballRenderItem->Geo->submeshList["Geo_Ball"].vertexBaseLocation;

		auto floorRenderItem = std::make_unique<RenderItem>();
		floorRenderItem->Geo = meshes["Geo"].get();
		floorRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		floorRenderItem->indexCount = floorRenderItem->Geo->submeshList["Geo_Gird"].indexCount;
		floorRenderItem->indexStartLocation = floorRenderItem->Geo->submeshList["Geo_Gird"].indexStartLocation;
		floorRenderItem->vertexBaseLocation = floorRenderItem->Geo->submeshList["Geo_Gird"].vertexBaseLocation;

		auto nailongRenderItem = std::make_unique<RenderItem>();
		nailongRenderItem->Geo = meshes["objGeo"].get();
		nailongRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		nailongRenderItem->indexCount = nailongRenderItem->Geo->submeshList["Nailong"].indexCount;
		nailongRenderItem->indexStartLocation = nailongRenderItem->Geo->submeshList["Nailong"].indexStartLocation;
		nailongRenderItem->vertexBaseLocation = nailongRenderItem->Geo->submeshList["Nailong"].vertexBaseLocation;
		
		opaqueRenderItems.emplace_back(std::move(cylinderRenderItem));
		opaqueRenderItems.emplace_back(std::move(ballRenderItem));
		opaqueRenderItems.emplace_back(std::move(floorRenderItem));
		opaqueRenderItems.emplace_back(std::move(nailongRenderItem));
	}

	// 天空盒渲染项
	{
		auto skyboxRenderItem = std::make_unique<RenderItem>();
		skyboxRenderItem->Geo = meshes["Geo"].get();
		skyboxRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		skyboxRenderItem->indexCount = skyboxRenderItem->Geo->submeshList["Geo_Ball"].indexCount;
		skyboxRenderItem->indexStartLocation = skyboxRenderItem->Geo->submeshList["Geo_Ball"].indexStartLocation;
		skyboxRenderItem->vertexBaseLocation = skyboxRenderItem->Geo->submeshList["Geo_Ball"].vertexBaseLocation;

		skycubeRenderItem.emplace_back(std::move(skyboxRenderItem));
	}

	// 透明渲染项
	{
		auto waveRenderItem = std::make_unique<RenderItem>();
		waveRenderItem->Geo = meshes["Geo"].get();
		waveRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		waveRenderItem->indexCount = waveRenderItem->Geo->submeshList["Geo_Gird"].indexCount;
		waveRenderItem->indexStartLocation = waveRenderItem->Geo->submeshList["Geo_Gird"].indexStartLocation;
		waveRenderItem->vertexBaseLocation = waveRenderItem->Geo->submeshList["Geo_Gird"].vertexBaseLocation;

		transparentRenderItems.emplace_back(std::move(waveRenderItem));
	}

	for (const auto& item : opaqueRenderItems)
		allRenderItems.emplace_back(item.get());
	for (const auto& item : skycubeRenderItem)
		allRenderItems.emplace_back(item.get());
	for (const auto& item : transparentRenderItems)
		allRenderItems.emplace_back(item.get());
}
void RenderApp::buildInstances()
{
	XMMATRIX leftCylinderWorld = XMMatrixTranslation(-2.5f, +0.0f, 0.0f);
	XMMATRIX rightCylinderWorld = XMMatrixTranslation(+2.5f, +0.0f, 0.0f);
	InstanceData leftCylinderInstance;
	XMStoreFloat4x4(&leftCylinderInstance.worldTransform, leftCylinderWorld);
	leftCylinderInstance.materialIndex = materials["Brick"].get()->materialIndex;
	leftCylinderInstance.textureTableIndex = 0;
	leftCylinderInstance.textureFlags = (UINT)(TextureType::TEX_DIFFUSE | TextureType::TEX_NORMAL | TextureType::TEX_DEPTH);
	InstanceData rightCylinderInstance;
	XMStoreFloat4x4(&rightCylinderInstance.worldTransform, rightCylinderWorld);
	rightCylinderInstance.materialIndex = materials["Brick"].get()->materialIndex;
	rightCylinderInstance.textureTableIndex = 0;
	rightCylinderInstance.textureFlags = (UINT)(TextureType::TEX_DIFFUSE | TextureType::TEX_NORMAL | TextureType::TEX_DEPTH);
	cylinderRenderItem->instances.push_back(leftCylinderInstance);
	cylinderRenderItem->instances.push_back(rightCylinderInstance);


	XMMATRIX rightBallWorld = XMMatrixTranslation(+2.5f, +2.96f, 0.0f);
	XMMATRIX leftBallWorld = XMMatrixTranslation(-2.5f, +2.96f, 0.0f);
	InstanceData leftBallInstance;
	XMStoreFloat4x4(&leftBallInstance.worldTransform, leftBallWorld);
	leftBallInstance.materialIndex = materials["Stone"].get()->materialIndex;
	leftBallInstance.textureTableIndex = 0;
	leftBallInstance.textureFlags = (UINT)TextureType::TEX_DIFFUSE;
	InstanceData rightBallInstance;
	XMStoreFloat4x4(&rightBallInstance.worldTransform, rightBallWorld);
	rightBallInstance.materialIndex = materials["Stone"].get()->materialIndex;
	rightBallInstance.textureTableIndex = 0;
	rightBallInstance.textureFlags = (UINT)TextureType::TEX_DIFFUSE;



	XMMATRIX floorWorld = XMMatrixScaling(1.5f, 1.5f, 1.5f) * XMMatrixTranslation(0.0f, -2.0f, 0.0f);
	XMMATRIX floorTextureTransform = XMMatrixScaling(5.0f, 5.0f, 5.0f);
	InstanceData floorInstance;
	XMStoreFloat4x4(&floorInstance.worldTransform, floorWorld);
	XMStoreFloat4x4(&floorInstance.textureTransform, floorTextureTransform);
	floorInstance.materialIndex = materials["Stone"].get()->materialIndex;
	floorInstance.textureTableIndex = 0;
	floorInstance.textureFlags = (UINT)(TextureType::TEX_DIFFUSE | TextureType::TEX_NORMAL | TextureType::TEX_DEPTH);
	floorRenderItem->instances.push_back(floorInstance);



	XMMATRIX nailongWorld = XMMatrixScaling(0.2f, 0.2f, 0.2f) * XMMatrixRotationNormal({ 0.0f,1.0f,0.0f }, MathHelper::Pi) * XMMatrixTranslation(0.0f, -1.0f, -0.0f);
	InstanceData nailongInstance;
	XMStoreFloat4x4(&nailongInstance.worldTransform, nailongWorld);
	nailongInstance.materialIndex = materials["Wood"].get()->materialIndex;
	nailongInstance.textureTableIndex = 0;
	nailongInstance.textureFlags = (UINT)TextureType::TEX_DIFFUSE;
	nailongRenderItem->instances.push_back(nailongInstance);

	XMMATRIX skyboxWorld = XMMatrixScaling(0.5f, 0.5f, 0.5f);

	InstanceData skyboxInstance;
	XMStoreFloat4x4(&skyboxInstance.worldTransform, skyboxWorld);
	skyboxInstance.materialIndex = materials["Skycube"].get()->materialIndex;
	skyboxInstance.textureTableIndex = 0;
	skyboxInstance.textureFlags = (UINT)TextureType::TEX_DIFFUSE;
	skyboxRenderItem->instances.push_back(skyboxInstance);


	XMMATRIX waveWorld = XMMatrixTranslation(0.0f, 0.0f, 0.0f);
	InstanceData waveInstance;
	XMStoreFloat4x4(&waveInstance.worldTransform, waveWorld);
	waveInstance.materialIndex = materials["Water"].get()->materialIndex;
	waveInstance.textureTableIndex = 0;
	waveInstance.textureFlags = (UINT)TextureType::TEX_DIFFUSE;
	waveRenderItem->instances.push_back(waveInstance);
}
// 创建帧资源
void RenderApp::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
		frameResources.emplace_back(std::make_unique<FrameResource>(d3dDevice.Get(), 1, (UINT)allRenderItems.size(), (UINT)materials.size()));
}
// 创建渲染管线状态对象
void RenderApp::BuildPSOs()
{
	CD3DX12_RASTERIZER_DESC drd(D3D12_DEFAULT);
	drd.FillMode = D3D12_FILL_MODE_SOLID;
	drd.CullMode = D3D12_CULL_MODE_BACK;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC OpaquePSODesc;
	ZeroMemory(&OpaquePSODesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	OpaquePSODesc.pRootSignature = rootSignature.Get();
	OpaquePSODesc.VS =
	{
		reinterpret_cast<BYTE*>(shaders["VS"]->GetBufferPointer()),
		shaders["VS"]->GetBufferSize()
	};
	OpaquePSODesc.PS =
	{
		reinterpret_cast<BYTE*>(shaders["PS"]->GetBufferPointer()),
		shaders["PS"]->GetBufferSize()
	};
	OpaquePSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	OpaquePSODesc.BlendState.AlphaToCoverageEnable = true;
	OpaquePSODesc.SampleMask = UINT_MAX;
	OpaquePSODesc.RasterizerState = drd;
	OpaquePSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	OpaquePSODesc.InputLayout = { inputLayout["Default"].data(),(UINT)inputLayout["Default"].size()};
	OpaquePSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	OpaquePSODesc.NumRenderTargets = 1;
	OpaquePSODesc.RTVFormats[0] = backBufferFormat;
	OpaquePSODesc.DSVFormat = depthStencilFormat;
	OpaquePSODesc.SampleDesc.Count = 1;
	OpaquePSODesc.SampleDesc.Quality = 0;
	OpaquePSODesc.NodeMask = 0;
	OpaquePSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&OpaquePSODesc, IID_PPV_ARGS(&PSOs["Solid"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaque4xPsoDesc = OpaquePSODesc;	//复制一份opaquePsoDesc，开启MSAA时用
	opaque4xPsoDesc.SampleDesc.Count = 4;	//采样数量设为4
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&opaque4xPsoDesc, IID_PPV_ARGS(&PSOs["opaque4x"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPSODesc = OpaquePSODesc;
	D3D12_RENDER_TARGET_BLEND_DESC transparentBlendDesc;
	transparentBlendDesc.BlendEnable = true;
	transparentBlendDesc.LogicOpEnable = false;
	transparentBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	transparentBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	transparentBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	transparentBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	transparentBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	transparentBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	transparentBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	transparentBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	transparentPSODesc.BlendState.RenderTarget[0] = transparentBlendDesc;
	transparentPSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&transparentPSODesc, IID_PPV_ARGS(&PSOs["Transparent"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC WavePSODesc = transparentPSODesc;
	WavePSODesc.VS =
	{
		reinterpret_cast<BYTE*>(shaders["VS_Wave"]->GetBufferPointer()),
		shaders["VS_Wave"]->GetBufferSize()
	};
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&WavePSODesc, IID_PPV_ARGS(&PSOs["Wave"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC WireframePSODesc = OpaquePSODesc;
	drd.FillMode = D3D12_FILL_MODE_WIREFRAME;
	WireframePSODesc.RasterizerState = drd;
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&WireframePSODesc, IID_PPV_ARGS(&PSOs["Wireframe"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC skycubePsoDesc = OpaquePSODesc;
	skycubePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	skycubePsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	skycubePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(shaders["VS_Skycube"]->GetBufferPointer()),
		shaders["VS_Skycube"]->GetBufferSize()
	};
	skycubePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(shaders["PS_Skycube"]->GetBufferPointer()),
		shaders["PS_Skycube"]->GetBufferSize()
	};
	skycubePsoDesc.InputLayout = { inputLayout["Skycube"].data(),(UINT)inputLayout["Skycube"].size() };
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&skycubePsoDesc, IID_PPV_ARGS(&PSOs["SkyCube"])));
}

void RenderApp::UpdateCameraState(const GameTimer& GTimer)
{
	cameraMotionParam.moveDirection = { 0,0,0 };
	cameraMotionParam.rollingDirection = 0;

	if (keyDown['W']) cameraMotionParam.moveDirection.z += 1;
	if (keyDown['S']) cameraMotionParam.moveDirection.z -= 1;
	if (keyDown['A']) cameraMotionParam.moveDirection.x -= 1;
	if (keyDown['D']) cameraMotionParam.moveDirection.x += 1;
	if (keyDown['R']) cameraMotionParam.moveDirection.y += 1;
	if (keyDown['F']) cameraMotionParam.moveDirection.y -= 1;

	if (keyDown['Q']) cameraMotionParam.rollingDirection -= 1;
	if (keyDown['E']) cameraMotionParam.rollingDirection += 1;

	if (DirectX::XMVectorGetX(DirectX::XMVector3Length(DirectX::XMLoadFloat3(&cameraMotionParam.moveDirection))) > 1e-3f)
	{
		float deltaTime = GTimer.DeltaTime();
		camera.moveForward_Backward(deltaTime * cameraMotionParam.moveDirection.z * cameraMotionParam.moveSpeed);
		camera.moveRight_left(deltaTime * cameraMotionParam.moveDirection.x * cameraMotionParam.moveSpeed);
		camera.fly_drop(deltaTime * cameraMotionParam.moveDirection.y * cameraMotionParam.moveSpeed);
		camera.rotate(deltaTime * cameraMotionParam.rollingDirection * cameraMotionParam.rollingSpeed, 0.0f, 0.0f);
	}
	else if (fabsf(cameraMotionParam.rollingDirection) > 1e-3f)
	{
		float deltaTime = GTimer.DeltaTime();
		camera.rotate(deltaTime * cameraMotionParam.rollingDirection * cameraMotionParam.rollingSpeed,0.0f,0.0f);
	}
}

// 更新物体常量缓冲区
void RenderApp::updateInstanceDataBuffers()
{
	auto currentInstanceDataBuffer = currentFrameResource->instanceStructuredBuffer.get();

	UINT instanceOffset = 0;
	for (auto& it : allRenderItems)
	{
		it->instanceBufferOffset = instanceOffset;

		for (size_t i = 0; i < it->instances.size(); ++i)
		{
			XMMATRIX worldTransform = XMLoadFloat4x4(&it->instances[i].worldTransform);
			XMMATRIX textureTransform = XMLoadFloat4x4(&it->instances[i].textureTransform);
			InstanceData instanceData = {};
			// 转置矩阵以符合HLSL的列主序要求：C++默认是行主序，HLSL默认是列主序
			XMStoreFloat4x4(&instanceData.worldTransform, XMMatrixTranspose(worldTransform));
			XMStoreFloat4x4(&instanceData.textureTransform, XMMatrixTranspose(textureTransform));
			instanceData.materialIndex = it->instances[i].materialIndex;
			instanceData.textureTableIndex = it->instances[i].textureTableIndex;
			instanceData.textureFlags = it->instances[i].textureFlags;

			currentInstanceDataBuffer->CopyData(instanceOffset + static_cast<UINT>(i), instanceData);
		}
		instanceOffset += static_cast<UINT>(it->instances.size());
	}
}

// 更新渲染过程常量
void RenderApp::updatePassConstBuffers()
{
	RenderingPassConstants mRenderingPassConstantsBuffer;

	XMMATRIX view = camera.getViewMatrixXM();
	XMMATRIX proj = camera.getProjMatrixXM();
	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&mRenderingPassConstantsBuffer.view, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mRenderingPassConstantsBuffer.proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mRenderingPassConstantsBuffer.viewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mRenderingPassConstantsBuffer.invView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mRenderingPassConstantsBuffer.invProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mRenderingPassConstantsBuffer.invViewProj, XMMatrixTranspose(invViewProj));

	mRenderingPassConstantsBuffer.eyePosW = camera.getPositionFloat3();
	mRenderingPassConstantsBuffer.renderTargetSize = XMFLOAT2((float)clientWidth, (float)clientHeight);
	mRenderingPassConstantsBuffer.invRenderTargetSize = XMFLOAT2(1.0f / static_cast<float>(clientWidth), 1.0f / static_cast<float>(clientHeight));
	mRenderingPassConstantsBuffer.nearZ = camera.getNearZ();
	mRenderingPassConstantsBuffer.farZ = camera.getFarZ();
	mRenderingPassConstantsBuffer.totalTime = gameTimer.TotalTime();
	mRenderingPassConstantsBuffer.deltaTime = gameTimer.DeltaTime();
	mRenderingPassConstantsBuffer.ambientIlluminating = { 0.2f,0.2f,0.2f,1.0f };

	Light dirLight;
	dirLight.rgbIntensity = { 1.0f,1.0f,1.0f };
	dirLight.direction = { -0.57735f,-0.57735f,0.57735f };

	Light pointLight;
	pointLight.rgbIntensity = { 2.0f,2.0f,2.0f };
	pointLight.end = 15.0f;
	pointLight.position = { 6.0f * sinf(gameTimer.TotalTime() * MathHelper::Pi / 16.0f),2.0f,6.0f * cosf(gameTimer.TotalTime() * MathHelper::Pi / 16.0f) };

	mRenderingPassConstantsBuffer.lights[0] = dirLight;
	mRenderingPassConstantsBuffer.lights[1] = pointLight;

	auto currentPassConstsBuffer = currentFrameResource->passConstBuffer.get();
	currentPassConstsBuffer->CopyData(0, mRenderingPassConstantsBuffer);
}

// 绘制渲染项
void RenderApp::DrawRenderItems(ID3D12GraphicsCommandList* commandList, const std::vector<std::unique_ptr<RenderItem>>& renderItems)const
{
	auto instanceStructuredBuffer = currentFrameResource->instanceStructuredBuffer->Resource();

	for (const auto& item : renderItems)
	{
		commandList->IASetVertexBuffers(0, 1, &item->Geo->VertexBufferView());
		commandList->IASetIndexBuffer(&item->Geo->IndexBufferView());
		commandList->IASetPrimitiveTopology(item->primitiveType);

		commandList->SetGraphicsRootConstantBufferView(0, instanceStructuredBuffer->GetGPUVirtualAddress());

		commandList->DrawIndexedInstanced(item->indexCount, (UINT)item->instances.size(), item->indexStartLocation, item->vertexBaseLocation, 0);
	}
}

// 获取指向MyApp类自身的指针
const RenderApp* RenderApp::getAppPtr()const
{
	return this;
}