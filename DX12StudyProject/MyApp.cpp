#include "MyApp.h"

using namespace DirectX;
using namespace Microsoft::WRL;

float CircleRun(float x, float c)
{
	float res = abs(fmod(x, 2*c));
	if (res < c)
		return res;
	else
		return 2 * c - res;
}

MyApp::MyApp(HINSTANCE hInstance) : DXApp(hInstance), windowClass(hInstance)
{
	mainWndTitle = L"Mayohoshi Render";
	d3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
	backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	clientWidth = 1000;
	clientHeight = 600;
}
MyApp::~MyApp()
{
	isAppRunning = false;
	isAppPaused = true;
	isRenderThreadRunning = false;
	isRenderPaused = true;

	if (renderThread.joinable())
		renderThread.join();
	if (controlThread.joinable())
		controlThread.join();

	if(d3dDevice!=nullptr)
		FlushCommandQueue();
}

// 消息过程处理函数
LRESULT MyApp::MessageProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
	case WM_MOUSEWHEEL:
		MouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
		return 0;
	case WM_KEYUP:
		if (wParam == VK_ESCAPE)
		{
			PostQuitMessage(0);
		}
		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

// 应用程序初始化
bool MyApp::Init()
{
	if(!DXApp::InitWindowClass(windowClass,L"Render Main Window Class"))
		return false;
	if (!DXApp::InitWindow(appMainWnd, windowClass, mainWndTitle,100,100,800,600))
		return false;
    if(!DXApp::InitDirectX3D())
		return false;
	Resize();

	ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr));
	LoadTexture(); 
	BuildRootSignature(); 
	BuildDescriptorHeaps(); 
	BuildShaders(); 
	BuildInputLayout(); 
	BuildMeshGeometry(); 
	BuildImportedGeometry(); 
	BuildMaterials(); 
	BuildRenderItems(); 
	BuildFrameResources(); 
	BuildPSOs(); 

	ThrowIfFailed(commandList->Close());
	ID3D12CommandList* cmdsLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
	FlushCommandQueue();

	isAppRunning = true;
	isAppPaused = false;
	isRenderThreadRunning = true;
	isRenderPaused = false;

	gameTimer.Reset();

	controlThread = std::thread(&MyApp::ControlLoop, this);
	renderThread = std::thread(&MyApp::RenderLoop, this);

	return true;
}

// 渲染线程循环
void MyApp::RenderLoop()
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
void MyApp::Resize()
{
	DXApp::Resize();

	// 设置视口
	screenViewport.Height = static_cast<float>(clientHeight);
	screenViewport.Width = static_cast<float>(clientWidth);
	screenViewport.TopLeftX = 0;
	screenViewport.TopLeftY = 0;
	screenViewport.MaxDepth = 1.0f;
	screenViewport.MinDepth = 0.0f;
	scissorRect = { 0,0,clientWidth,clientHeight };

	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, W_H_Ratio(), 1.0f, 100.0f);
	XMStoreFloat4x4(&projectionTransform, P);
}

// 更新帧画面
void MyApp::Update(const GameTimer& GTimer)
{
	ChangePSOstate();
	UpdateCamera();

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

	UpdateMaterialConstBuffers();
	UpdateObjectsConstBuffers();
	UpdatePassConstBuffers();
}

// 绘制帧画面
void MyApp::Draw(const GameTimer& GTimer)
{
	auto cmdListAllocator = currentFrameResource->commandAllocator;

	ThrowIfFailed(cmdListAllocator->Reset());
	if (isWireframeEnabled)
		commandList->Reset(cmdListAllocator.Get(), PSOs["Wireframe"].Get());
	else
		commandList->Reset(cmdListAllocator.Get(), PSOs["Solid"].Get());

	commandList->RSSetViewports(1, &screenViewport);
	commandList->RSSetScissorRects(1, &scissorRect);

	commandList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	commandList->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::Black, 0, nullptr);
	commandList->ClearDepthStencilView(DepthStencilBufferView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	commandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilBufferView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	commandList->SetGraphicsRootSignature(rootSignature.Get());

	auto passConstBuffer = currentFrameResource->passConstBuffer->Resource();
	commandList->SetGraphicsRootConstantBufferView(3, passConstBuffer->GetGPUVirtualAddress());

	DrawRenderItems(commandList.Get(), opaqueRenderItems);

	commandList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(commandList->Close());

	ID3D12CommandList* CommandList[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(CommandList), CommandList); 

	ThrowIfFailed(swapChain->Present(0, 0));

	currentBackBuffer = (currentBackBuffer + 1) % SwapChainBufferCount;
	currentFrameResource->fence = ++currentFenceValue;
	commandQueue->Signal(fence.Get(), currentFenceValue);
}

// 当鼠标按下时调用
void MyApp::MouseDown(WPARAM ButtonState, int x, int y)
{
	lastMousePosition.x = x;
	lastMousePosition.y = y;

	SetCapture(mainWndHwnd);
}
// 当鼠标抬起时调用
void MyApp::MouseUp(WPARAM ButtonState, int x, int y)
{
	ReleaseCapture();
}
// 当鼠标移动时调用
void MyApp::MouseMove(WPARAM ButtonState, int x, int y)
{
	if ((ButtonState & MK_LBUTTON) != 0)
	{
		float dTheta = XMConvertToRadians(0.25f * static_cast<float>(x - lastMousePosition.x));
		float dPhi = XMConvertToRadians(0.25f * static_cast<float>(y - lastMousePosition.y));

		theta += dTheta;
		phi += dPhi;
	}

	lastMousePosition.x = x;
	lastMousePosition.y = y;
}
// 当鼠标滚轮滚动时
void MyApp::MouseWheel(short zDelta)
{
	radius += -0.05f * (zDelta / 10);

	radius = MathHelper::Clamp(radius, 3.0f, 15.0f);
}

// 载入纹理
void MyApp::LoadTexture()
{
	auto texStone = std::make_unique<Texture>();
	texStone->name = "stone";
	texStone->filename = L"../Resources/Textures/stone.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(
		d3dDevice.Get(), commandList.Get(), texStone->filename.c_str(), texStone->resource, texStone->uploadHeap));

	auto texBrick = std::make_unique<Texture>();
	texBrick->name = "brick";
	texBrick->filename = L"../Resources/Textures/bricks.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(
		d3dDevice.Get(), commandList.Get(), texBrick->filename.c_str(), texBrick->resource, texBrick->uploadHeap));

	textures[texStone->name] = std::move(texStone);
	textures[texBrick->name] = std::move(texBrick);
}
// 创建根签名
void MyApp::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE textureTable;
	textureTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_ROOT_PARAMETER slotRootParameter[4];
	slotRootParameter[0].InitAsDescriptorTable(1, &textureTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[1].InitAsConstantBufferView(0);
	slotRootParameter[2].InitAsConstantBufferView(1);
	slotRootParameter[3].InitAsConstantBufferView(2);

	auto staticSamplers = DXBase::GetStaticSamplers();

	CD3DX12_ROOT_SIGNATURE_DESC RootSignatureDesc(4, slotRootParameter, (UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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
void MyApp::BuildDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC SRV_HEAP_DESC;
	SRV_HEAP_DESC.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	SRV_HEAP_DESC.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	SRV_HEAP_DESC.NumDescriptors = 2;
	SRV_HEAP_DESC.NodeMask = 0;
	ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&SRV_HEAP_DESC, IID_PPV_ARGS(&srvDescriptorHeap)));

	CD3DX12_CPU_DESCRIPTOR_HANDLE srvCPUHandle(srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	auto stoneTex = textures["stone"]->resource;
	auto brickTex = textures["brick"]->resource;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = stoneTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;
	d3dDevice->CreateShaderResourceView(stoneTex.Get(), &srvDesc, srvCPUHandle);

	srvCPUHandle.Offset(1, cbs_srv_uavDescriptorSize);

	srvDesc.Format = brickTex->GetDesc().Format;
	d3dDevice->CreateShaderResourceView(brickTex.Get(), &srvDesc, srvCPUHandle);
}
// 编译着色器
void MyApp::BuildShaders()
{
	shaders["VS"] = DXBase::CompileShaderOnline(L"..\\Shaders\\Main.hlsl", nullptr, "VS", "vs_5_1");
	shaders["PS"] = DXBase::CompileShaderOnline(L"..\\Shaders\\Main.hlsl", nullptr, "PS", "ps_5_1");
}
// 创建输入布局
void MyApp::BuildInputLayout()
{
	inputLayout =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	};
}
// 创建网格体
void MyApp::BuildMeshGeometry()
{
	GeometryGenerator GeoGenerator;
	GeometryGenerator::MeshData cylinder = GeoGenerator.CreateCylinder(1.0f, 0.56f, 4.0f, 100, 20);
	GeometryGenerator::MeshData ball = GeoGenerator.CreateBall(1.0f, 50, 50);
	
	UINT CylinderVertexOffset = 0;
	auto BallVertexOffset = (UINT)cylinder.Vertices.size();
	UINT CylinderIndexOffset = 0;
	auto BallIndexOffset = (UINT)cylinder.Indices_32.size();

	SubmeshGeometry Geo_Cylinder;
	Geo_Cylinder.name = "Geo_Cylinder";
	Geo_Cylinder.vertexBaseLocation = CylinderVertexOffset;
	Geo_Cylinder.indexStartLocation = CylinderIndexOffset;
	Geo_Cylinder.indexCount = (UINT)cylinder.Indices_32.size();
	SubmeshGeometry Geo_Ball;
	Geo_Ball.name = "Geo_Ball";
	Geo_Ball.vertexBaseLocation = BallVertexOffset;
	Geo_Ball.indexStartLocation = BallIndexOffset;
	Geo_Ball.indexCount = (UINT)ball.Indices_32.size();

	auto totalVertexCount = ball.Vertices.size() + cylinder.Vertices.size();

	std::vector<VertexConstants> vertices(totalVertexCount);
	std::vector<std::uint16_t> indices;
	UINT k = 0;
	for (size_t i = 0; i < cylinder.Vertices.size(); ++i,++k)
	{
		vertices[k].pos = cylinder.Vertices[i].position;
		vertices[k].normal = cylinder.Vertices[i].Normal;
		vertices[k].texture = cylinder.Vertices[i].Texture;
	}
	for (size_t i = 0; i < ball.Vertices.size(); ++i,++k)
	{
		vertices[k].pos = ball.Vertices[i].position;
		vertices[k].normal = ball.Vertices[i].Normal;
		vertices[k].texture = ball.Vertices[i].Texture;
	}

	indices.insert(indices.end(), std::begin(cylinder.GetIndices_16()), std::end(cylinder.GetIndices_16()));
	indices.insert(indices.end(), std::begin(ball.GetIndices_16()), std::end(ball.GetIndices_16()));

	const UINT vertexBufferByteSize = (UINT)vertices.size() * sizeof(VertexConstants);
	const UINT indexBufferByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto Geo = std::make_unique<MeshGeometry>();
	Geo->name = "Geo";

	ThrowIfFailed(D3DCreateBlob(vertexBufferByteSize, &Geo->vertexBufferCPU));
	CopyMemory(Geo->vertexBufferCPU->GetBufferPointer(), vertices.data(), vertexBufferByteSize);
	ThrowIfFailed(D3DCreateBlob(indexBufferByteSize, &Geo->indexBufferCPU));
	CopyMemory(Geo->indexBufferCPU->GetBufferPointer(), indices.data(), indexBufferByteSize);

	Geo->vertexBufferGPU = DXBase::CreateDefaultBuffer(d3dDevice.Get(), commandList.Get(), vertices.data(), vertexBufferByteSize, Geo->vertexBufferUploader);
	Geo->indexBufferGPU = DXBase::CreateDefaultBuffer(d3dDevice.Get(), commandList.Get(), indices.data(), indexBufferByteSize, Geo->indexBufferUploader);

	Geo->vertexByteStride = sizeof(VertexConstants);
	Geo->vertexBufferByteSize = vertexBufferByteSize;
	Geo->indexFormat = DXGI_FORMAT_R16_UINT;
	Geo->indexBufferByteSize = indexBufferByteSize;

	Geo->submeshList[Geo_Cylinder.name] = Geo_Cylinder;
	Geo->submeshList[Geo_Ball.name] = Geo_Ball;

	geos[Geo->name] = std::move(Geo);
}
void MyApp::BuildImportedGeometry()
{
	std::ifstream fin("../Resources/Models/skull.txt");

	if (!fin)
	{
		MessageBox(nullptr, L"../Resources/Models/skull.txt not found.", nullptr, 0);
		return;
	}

	UINT vcount = 0;
	UINT tcount = 0;
	std::string ignore;

	fin >> ignore >> vcount;
	fin >> ignore >> tcount;
	fin >> ignore >> ignore >> ignore >> ignore;

	std::vector<VertexConstants> vertices(vcount);
	for (UINT i = 0; i < vcount; ++i)
	{
		fin >> vertices[i].pos.x >> vertices[i].pos.y >> vertices[i].pos.z;
		fin >> vertices[i].normal.x >> vertices[i].normal.y >> vertices[i].normal.z;
	}

	fin >> ignore;
	fin >> ignore;
	fin >> ignore;

	std::vector<std::int32_t> indices(3 * tcount);
	for (UINT i = 0; i < tcount; ++i)
	{
		fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
	}

	fin.close();

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(VertexConstants);

	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::int32_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->name = "skullGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->vertexBufferCPU));
	CopyMemory(geo->vertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->indexBufferCPU));
	CopyMemory(geo->indexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->vertexBufferGPU = DXBase::CreateDefaultBuffer(d3dDevice.Get(),
		commandList.Get(), vertices.data(), vbByteSize, geo->vertexBufferUploader);

	geo->indexBufferGPU = DXBase::CreateDefaultBuffer(d3dDevice.Get(),
		commandList.Get(), indices.data(), ibByteSize, geo->indexBufferUploader);

	geo->vertexByteStride = sizeof(VertexConstants);
	geo->vertexBufferByteSize = vbByteSize;
	geo->indexFormat = DXGI_FORMAT_R32_UINT;
	geo->indexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.indexCount = (UINT)indices.size();
	submesh.indexStartLocation = 0;
	submesh.vertexBaseLocation = 0;

	geo->submeshList["skull"] = submesh;

	geos[geo->name] = std::move(geo);
}
// 创建材质
void MyApp::BuildMaterials()
{
	UINT MaterialIndex = 0;

	auto matGrass = std::make_unique<Material>();
	matGrass->name = "Grass";
	matGrass->materialConstBufferIndex = MaterialIndex;
	matGrass->diffuseSrvHeapIndex = MaterialIndex++;
	matGrass->numDirtyFrames = gNumFrameResources;
	matGrass->diffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	matGrass->fresneRf0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	matGrass->roughness = 0.5f;
	auto matGlass = std::make_unique<Material>();
	matGlass->name = "Glass";
	matGlass->materialConstBufferIndex = MaterialIndex;
	matGlass->diffuseSrvHeapIndex = MaterialIndex++;
	matGlass->numDirtyFrames = gNumFrameResources;
	matGlass->diffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	matGlass->fresneRf0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	matGlass->roughness = 0.02f;

	materials[matGrass->name] = std::move(matGrass);
	materials[matGlass->name] = std::move(matGlass);
}
// 创建渲染项
void MyApp::BuildRenderItems()
{
	auto leftCylinderRenderItem = std::make_unique<RenderItem>();
	auto leftBallRenderItem = std::make_unique<RenderItem>();
	auto rightCylinderRenderItem = std::make_unique<RenderItem>();
	auto rightBallRenderItem = std::make_unique<RenderItem>();

	UINT GeoObjectIndex = 0;

	XMMATRIX leftCylinderWorld = XMMatrixTranslation(0.0f, +0.0f, -2.0f);
	XMMATRIX leftBallWorld = XMMatrixTranslation(0.0f, +2.96f, -2.0f);
	XMMATRIX rightCylinderWorld = XMMatrixTranslation(0.0f, +0.0f ,+2.0f );
	XMMATRIX rightBallWorld = XMMatrixTranslation(0.0f, +2.96f, +2.0f);

	XMStoreFloat4x4(&leftCylinderRenderItem->worldTransform, leftCylinderWorld);
	leftCylinderRenderItem->objectConstBufferIndex = GeoObjectIndex++;
	leftCylinderRenderItem->material = materials["Grass"].get();
	leftCylinderRenderItem->Geo = geos["Geo"].get();
	leftCylinderRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	leftCylinderRenderItem->indexCount = leftCylinderRenderItem->Geo->submeshList["Geo_Cylinder"].indexCount;
	leftCylinderRenderItem->indexStartLocation = leftCylinderRenderItem->Geo->submeshList["Geo_Cylinder"].indexStartLocation;
	leftCylinderRenderItem->vertexBaseLocation = leftCylinderRenderItem->Geo->submeshList["Geo_Cylinder"].vertexBaseLocation;

	XMStoreFloat4x4(&leftBallRenderItem->worldTransform, leftBallWorld);
	leftBallRenderItem->objectConstBufferIndex = GeoObjectIndex++;
	leftBallRenderItem->material = materials["Grass"].get();
	leftBallRenderItem->Geo = geos["Geo"].get();
	leftBallRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	leftBallRenderItem->indexCount = leftBallRenderItem->Geo->submeshList["Geo_Ball"].indexCount;
	leftBallRenderItem->indexStartLocation = leftBallRenderItem->Geo->submeshList["Geo_Ball"].indexStartLocation;
	leftBallRenderItem->vertexBaseLocation = leftBallRenderItem->Geo->submeshList["Geo_Ball"].vertexBaseLocation;

	XMStoreFloat4x4(&rightCylinderRenderItem->worldTransform, rightCylinderWorld);
	rightCylinderRenderItem->objectConstBufferIndex = GeoObjectIndex++;
	rightCylinderRenderItem->material = materials["Grass"].get();
	rightCylinderRenderItem->Geo = geos["Geo"].get();
	rightCylinderRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	rightCylinderRenderItem->indexCount = rightCylinderRenderItem->Geo->submeshList["Geo_Cylinder"].indexCount;
	rightCylinderRenderItem->indexStartLocation = rightCylinderRenderItem->Geo->submeshList["Geo_Cylinder"].indexStartLocation;
	rightCylinderRenderItem->vertexBaseLocation = rightCylinderRenderItem->Geo->submeshList["Geo_Cylinder"].vertexBaseLocation;

	XMStoreFloat4x4(&rightBallRenderItem->worldTransform, rightBallWorld);
	rightBallRenderItem->objectConstBufferIndex = GeoObjectIndex++;
	rightBallRenderItem->material = materials["Grass"].get();
	rightBallRenderItem->Geo = geos["Geo"].get();
	rightBallRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	rightBallRenderItem->indexCount = rightBallRenderItem->Geo->submeshList["Geo_Ball"].indexCount;
	rightBallRenderItem->indexStartLocation = rightBallRenderItem->Geo->submeshList["Geo_Ball"].indexStartLocation;
	rightBallRenderItem->vertexBaseLocation = rightBallRenderItem->Geo->submeshList["Geo_Ball"].vertexBaseLocation;

	auto skullRenderItem = std::make_unique<RenderItem>();
	XMMATRIX skullWorld = XMMatrixScaling(0.25f, 0.25f, 0.25f)* XMMatrixRotationNormal({ 0.0f,1.0f,0.0f }, 3*MathHelper::Pi / 2);

	XMStoreFloat4x4(&skullRenderItem->worldTransform, skullWorld);
	skullRenderItem->objectConstBufferIndex = GeoObjectIndex++;
	skullRenderItem->material = materials["Glass"].get();
	skullRenderItem->Geo = geos["skullGeo"].get();
	skullRenderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	skullRenderItem->indexCount = skullRenderItem->Geo->submeshList["skull"].indexCount;
	skullRenderItem->indexStartLocation = skullRenderItem->Geo->submeshList["skull"].indexStartLocation;
	skullRenderItem->vertexBaseLocation = skullRenderItem->Geo->submeshList["skull"].vertexBaseLocation;

	allRenderItems.push_back(std::move(leftCylinderRenderItem));
	allRenderItems.push_back(std::move(leftBallRenderItem));
	allRenderItems.push_back(std::move(rightCylinderRenderItem));
	allRenderItems.push_back(std::move(rightBallRenderItem));
	allRenderItems.push_back(std::move(skullRenderItem));

	for (auto& i: allRenderItems)
		opaqueRenderItems.push_back(i.get());
}
// 创建帧资源
void MyApp::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
		frameResources.push_back(std::make_unique<FrameResource>(d3dDevice.Get(), 1, (UINT)allRenderItems.size(), (UINT)materials.size()));
}
// 创建渲染管线状态对象
void MyApp::BuildPSOs()
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
	OpaquePSODesc.SampleMask = UINT_MAX;
	OpaquePSODesc.RasterizerState = drd;
	OpaquePSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	OpaquePSODesc.InputLayout = { inputLayout.data(),(UINT)inputLayout.size() };
	OpaquePSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	OpaquePSODesc.NumRenderTargets = 1;
	OpaquePSODesc.RTVFormats[0] = backBufferFormat;
	OpaquePSODesc.DSVFormat = depthStencilFormat;
	OpaquePSODesc.SampleDesc.Count = isMSAA4xOn ? 4 : 1;
	OpaquePSODesc.SampleDesc.Quality = isMSAA4xOn ? (MSAA4xQualityLevel - 1) : 0;
	OpaquePSODesc.NodeMask = 0;
	OpaquePSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&OpaquePSODesc, IID_PPV_ARGS(&PSOs["Solid"])));

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
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&transparentPSODesc, IID_PPV_ARGS(&PSOs["Transparent"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC WireframePSODesc = OpaquePSODesc;
	drd.FillMode = D3D12_FILL_MODE_WIREFRAME;
	WireframePSODesc.RasterizerState = drd;
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&WireframePSODesc, IID_PPV_ARGS(&PSOs["Wireframe"])));
}

// 更改PSO
void MyApp::ChangePSOstate()
{
	if (GetAsyncKeyState('1') & 0x8000)
		isWireframeEnabled = true;
	else
		isWireframeEnabled = false;
}
// 更新摄像头矩阵
void MyApp::UpdateCamera()
{
	eyePosition.x = radius * sinf(phi) * cosf(theta);
	eyePosition.z = radius * sinf(phi) * sinf(theta);
	eyePosition.y = radius * cosf(phi);

	XMVECTOR pos = XMVectorSet(eyePosition.x, eyePosition.y, eyePosition.z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up;
	if (sinf(phi) >= 0)
		up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	else
		up = XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&viewTransform, view);
}
// 更新物体常量缓冲区（世界矩阵）
void MyApp::UpdateObjectsConstBuffers()const
{
	auto currentObjectConstBuffer = currentFrameResource->objectConstBuffer.get();

	for (auto& it : allRenderItems)
	{
		if (it->numDirtyFrames > 0)
		{
			XMMATRIX worldTransform = XMLoadFloat4x4(&it->worldTransform);
			XMMATRIX textureTransform = XMLoadFloat4x4(&it->textureTransform);

			ObjectConstants objectconstant;
			XMStoreFloat4x4(&objectconstant.worldTransform,  XMMatrixTranspose(worldTransform));
			XMStoreFloat4x4(&objectconstant.textureTransform, XMMatrixTranspose(textureTransform));

			currentObjectConstBuffer->CopyData(it->objectConstBufferIndex, objectconstant);

			it->numDirtyFrames--;
		}
	}
}
// 更新渲染过程常量
void MyApp::UpdatePassConstBuffers()const
{
	RenderingPassConstants mRenderingPassConstantsBuffer;

	XMMATRIX view = XMLoadFloat4x4(&viewTransform);
	XMMATRIX proj = XMLoadFloat4x4(&projectionTransform);
	XMMATRIX viewProj=XMMatrixMultiply(view,proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&mRenderingPassConstantsBuffer.view, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mRenderingPassConstantsBuffer.proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mRenderingPassConstantsBuffer.viewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mRenderingPassConstantsBuffer.invView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mRenderingPassConstantsBuffer.invProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mRenderingPassConstantsBuffer.invViewProj, XMMatrixTranspose(invViewProj));

	mRenderingPassConstantsBuffer.eyePosW = eyePosition;
	mRenderingPassConstantsBuffer.renderTargetSize = XMFLOAT2((float)clientWidth, (float)clientHeight);
	mRenderingPassConstantsBuffer.invRenderTargetSize = XMFLOAT2(1.0f / clientWidth, 1.0f / clientHeight);
	mRenderingPassConstantsBuffer.nearZ = 1.0f;
	mRenderingPassConstantsBuffer.farZ = 100.0f;
	mRenderingPassConstantsBuffer.totalTime = gameTimer.TotalTime();
	mRenderingPassConstantsBuffer.deltaTime = gameTimer.DeltaTime();
	mRenderingPassConstantsBuffer.ambientIlluminating = { 0.2f,0.2f,0.2f,1.0f };

	mRenderingPassConstantsBuffer.lights[0].rgbIntensity = { 1.0f,1.0f,1.0f };
	mRenderingPassConstantsBuffer.lights[0].position = { 10.0f*sinf(gameTimer.TotalTime()*MathHelper::Pi),0.0f,0.0f};
	mRenderingPassConstantsBuffer.lights[0].direction = { 1.0f,0.0f,0.0f };

	auto currentPassConstsBuffer = currentFrameResource->passConstBuffer.get();
	currentPassConstsBuffer->CopyData(0, mRenderingPassConstantsBuffer);
}
// 更新材质常量缓冲区
void MyApp::UpdateMaterialConstBuffers()const
{
	auto currentMaterialConstBuffer = currentFrameResource->materialConstBuffer.get();

	for (auto& it : materials)
	{
		Material* mat = it.second.get();
		if(mat->numDirtyFrames > 0)
		{
			MaterialConstants materialConstant;
			materialConstant.diffuseAlbedo = mat->diffuseAlbedo;
			materialConstant.fresneRf0 = mat->fresneRf0;
			materialConstant.roughness = mat->roughness;
			XMStoreFloat4x4(&materialConstant.materialTransform, XMMatrixTranspose(XMLoadFloat4x4(&mat->materialTransform)));
			currentMaterialConstBuffer->CopyData(mat->materialConstBufferIndex, materialConstant);

			mat->numDirtyFrames--;
		}
	}
}

// 绘制渲染项
void MyApp::DrawRenderItems(ID3D12GraphicsCommandList* commandList, const std::vector<RenderItem*>& renderItems)const
{
	UINT objectConstBufferByteSize = DXBase::ConstUploadBufferByteSize256Alignment(sizeof(ObjectConstants));
	UINT materialConstBufferByteSize = DXBase::ConstUploadBufferByteSize256Alignment(sizeof(MaterialConstants));

	auto objectConstBuffer = currentFrameResource->objectConstBuffer->Resource();
	auto materialConstBuffer = currentFrameResource->materialConstBuffer->Resource();
	for (size_t itemIndex = 0; itemIndex < renderItems.size(); itemIndex++)
	{
		auto item = renderItems[itemIndex];
		commandList->IASetVertexBuffers(0, 1, &item->Geo->VertexBufferView());
		commandList->IASetIndexBuffer(&item->Geo->IndexBufferView());
		commandList->IASetPrimitiveTopology(item->primitiveType);

		CD3DX12_GPU_DESCRIPTOR_HANDLE tex(srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		tex.Offset(item->material->diffuseSrvHeapIndex, cbs_srv_uavDescriptorSize);

		D3D12_GPU_VIRTUAL_ADDRESS objectConstBufferAddress = objectConstBuffer->GetGPUVirtualAddress() + item->objectConstBufferIndex * objectConstBufferByteSize;
		D3D12_GPU_VIRTUAL_ADDRESS materialConstBufferAddress = materialConstBuffer->GetGPUVirtualAddress() + item->material->materialConstBufferIndex * materialConstBufferByteSize;

		commandList->SetGraphicsRootDescriptorTable(0, tex);
		commandList->SetGraphicsRootConstantBufferView(1, objectConstBufferAddress);
		commandList->SetGraphicsRootConstantBufferView(2, materialConstBufferAddress);

		commandList->DrawIndexedInstanced(item->indexCount, 1, item->indexStartLocation, item->vertexBaseLocation, 0);
	}
}

// 获取指向MyApp类自身的指针
const MyApp* MyApp::GetMyApp()const
{
	return this;
}