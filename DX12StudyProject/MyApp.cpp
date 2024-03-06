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

MyApp::MyApp(HINSTANCE hInstance) : DXApp(hInstance), WC1(hInstance)
{
	mMainWndTitle = L"Rikki-Rana-Render";
	md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
	mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	mClientWidth = 1000;
	mClientHeight = 600;
}
//消息过程处理函数
LRESULT MyApp::MessageProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			mAppPaused = true;
			mGameTimer.Stop();
		}
		else
		{
			mAppPaused = false;
			mGameTimer.Start();
		}
		return 0;
	case WM_SIZE:
		mClientWidth = LOWORD(lParam);
		mClientHeight = HIWORD(lParam);
		if (md3dDevice)
		{
			if (wParam == SIZE_MINIMIZED)
			{
				mAppPaused = true;
				mMinimized = true;
				mMaximized = false;
			}
			else if (wParam == SIZE_MAXIMIZED)
			{
				mAppPaused = false;
				mMinimized = false;
				mMaximized = true;
				Resize();
			}
			else if (wParam == SIZE_RESTORED)
			{
				if (mMinimized)
				{
					mAppPaused = false;
					mMinimized = false;
					Resize();
				}
				else if (mMaximized)
				{
					mAppPaused = false;
					mMaximized = false;
					Resize();
				}
				else if (mResized)
				{
					Resize();
				}
				else
				{
					Resize();
				}
			}
		}
		return 0;
	case WM_ENTERSIZEMOVE:
		mAppPaused = true;
		mResized = true;
		mGameTimer.Stop();
		return 0;
	case WM_EXITSIZEMOVE:
		mAppPaused = false;
		mResized = false;
		mGameTimer.Start();
		Resize();
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_MENUCHAR:
		return MAKELRESULT(0, MNC_CLOSE);
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
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
		else if ((int)wParam == VK_F2)
			Set4xMSAAState(!m4xMSAAState);
		return 0;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}
//应用程序初始化
bool MyApp::Init()
{
	if(!DXApp::InitWindowClass(WC1,L"JustTest"))
		return false;
	if (!DXApp::InitWindow(AppMainWin, WC1, mMainWndTitle,100,100,800,600))
		return false;
    if(!DXApp::InitDirectX3D())
		return false;
	Resize();

	ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), nullptr))

	BuildRootSignature();
	BuildShaders();
	BuildInputLayout();
	BuildMeshGeometry();
	BuildMaterials();
	BuildRenderItems();
	BuildFrameResources();
	BuildDescriptorHeaps();
	BuildConstantBufferViews();
	BuildPSOs();

	ThrowIfFailed(mCommandList->Close())
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
	FlushCommandQueue();

	return true;
}
//窗口大小重新适配
void MyApp::Resize()
{
	DXApp::Resize();

	//设置视口
	mScreenViewport.Height = static_cast<float>(mClientHeight);
	mScreenViewport.Width = static_cast<float>(mClientWidth);
	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.MaxDepth = 1.0f;
	mScreenViewport.MinDepth = 0.0f;
	mScissorRect = { 0,0,mClientWidth,mClientHeight };

	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, W_H_Ratio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}
//更新帧画面
void MyApp::Update(const GameTimer& GTimer)
{
	ChangePSOstate();
	UpdateCamara();

	mCurrentFrameResourceIndex = (mCurrentFrameResourceIndex + 1) % gNumFrameResources;
	mCurrentFrameResource = mFrameResources[mCurrentFrameResourceIndex].get();

	if (mCurrentFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrentFrameResource->Fence)
	{
		HANDLE event = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFrameResource->Fence, event))
		if (event)
		{
			WaitForSingleObject(event, INFINITE);
			CloseHandle(event);
		}
	}

	RGBA[0] = CircleRun(GTimer.TotalTime() * 0.05f, 1.0f);
	RGBA[1] = CircleRun(GTimer.TotalTime() * 0.07f, 1.0f);
	RGBA[2] = CircleRun(GTimer.TotalTime() * 0.11f, 1.0f);

	UpdateMaterialConstBuffers();
	UpdateObjectsConstBuffers();
	UpdatePassConstBuffers();

	std::ostringstream os;
	os << "Rikki-Rana-Render " << "FPS:" << FPS;

	SetWindowText(mhWndHwnd, AnsiToWstring(os.str().c_str()).c_str());
}
//绘制帧画面
void MyApp::Draw(const GameTimer& GTimer)
{
	auto cmdListAllocator = mCurrentFrameResource->CommandAllocator;

	ThrowIfFailed(cmdListAllocator->Reset())
	if (mIsWireframe)
		mCommandList->Reset(cmdListAllocator.Get(), mPSOs["Wireframe"].Get());
	else
		mCommandList->Reset(cmdListAllocator.Get(), mPSOs["Solid"].Get());

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	mCommandList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), RGBA, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilBufferView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilBufferView());

	ID3D12DescriptorHeap* DescriptorHeaps[] = { mCbvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(DescriptorHeaps), DescriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
	auto PassCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	PassCbvHandle.Offset(mPassCbvOffset + mCurrentFrameResourceIndex, mCBV_SRV_UAVDescriptorSize);
	mCommandList->SetGraphicsRootDescriptorTable(1, PassCbvHandle);

	DrawRenderItems(mCommandList.Get(), mOpaqueRenderItems);

	mCommandList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(mCommandList->Close())

	ID3D12CommandList* CommandList[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(CommandList), CommandList);

	ThrowIfFailed(mSwapChain->Present(0, 0))

	mCurrentBackBuffer = (mCurrentBackBuffer + 1) % SwapChainBufferCount;
	mCurrentFrameResource->Fence = ++mCurrentFence;
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}
//当鼠标按下时调用
void MyApp::MouseDown(WPARAM ButtonState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhWndHwnd);
}
//当鼠标抬起时调用
void MyApp::MouseUp(WPARAM ButtonState, int x, int y)
{
	ReleaseCapture();
}
//当鼠标移动时调用
void MyApp::MouseMove(WPARAM ButtonState, int x, int y)
{
	if ((ButtonState & MK_LBUTTON) != 0)
	{
		float dTheta = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
		float dPhi = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		mTheta += dTheta;
		mPhi += dPhi;
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}
//当鼠标滚轮滚动时
void MyApp::MouseWheel(short zDelta)
{
	mRadius += -0.05f * (zDelta / 10);

	mRadius = MathHelper::Clamp(mRadius, 3.0f, 15.0f);
}

//创建根签名
void MyApp::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE CBVTable0;
	CBVTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	CD3DX12_DESCRIPTOR_RANGE CBVTable1;
	CBVTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);
	CD3DX12_DESCRIPTOR_RANGE CBVTable2;
	CBVTable2.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);
	CD3DX12_ROOT_PARAMETER slotRootParameter[3];
	slotRootParameter[0].InitAsDescriptorTable(1, &CBVTable0);
	slotRootParameter[1].InitAsDescriptorTable(1, &CBVTable1);
	slotRootParameter[2].InitAsDescriptorTable(1, &CBVTable2);

	CD3DX12_ROOT_SIGNATURE_DESC RootSignatureDesc(3, slotRootParameter, 0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> SerializedRootSignature = nullptr;
	ComPtr<ID3DBlob> ErrorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		SerializedRootSignature.GetAddressOf(), ErrorBlob.GetAddressOf());

	if (ErrorBlob != nullptr)
		::OutputDebugStringA((char*)ErrorBlob->GetBufferPointer());
	ThrowIfFailed(hr)

	ThrowIfFailed(md3dDevice->CreateRootSignature(0, SerializedRootSignature->GetBufferPointer(),
		SerializedRootSignature->GetBufferSize(), IID_PPV_ARGS(&mRootSignature)))
}
//着色器，启动！
void MyApp::BuildShaders()
{
	mShaders["VS"] = DXBase::CompileShaderOnline(L"..\\Shaders\\Color.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["PS"] = DXBase::CompileShaderOnline(L"..\\Shaders\\Color.hlsl", nullptr, "PS", "ps_5_1");
}
//创建输入布局
void MyApp::BuildInputLayout()
{
	mInputLayout =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	};
}
//创建网格体
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
	Geo_Cylinder.VertexBaseLocation = CylinderVertexOffset;
	Geo_Cylinder.IndexStartLocation = CylinderIndexOffset;
	Geo_Cylinder.IndexCount = (UINT)cylinder.Indices_32.size();
	SubmeshGeometry Geo_Ball;
	Geo_Ball.name = "Geo_Ball";
	Geo_Ball.VertexBaseLocation = BallVertexOffset;
	Geo_Ball.IndexStartLocation = BallIndexOffset;
	Geo_Ball.IndexCount = (UINT)ball.Indices_32.size();

	auto totalVertexCount =
		ball.Vertices.size() +
		cylinder.Vertices.size();

	std::vector<VertexInfo> vertices(totalVertexCount);
	std::vector<std::uint16_t> indices;
	UINT k = 0;
	for (size_t i = 0; i < cylinder.Vertices.size(); i++,k++)
	{
		vertices[k].Pos = cylinder.Vertices[i].Position;
		vertices[k].Color = XMFLOAT4(DirectX::Colors::DarkGreen);
	}
	for (size_t i = 0; i < ball.Vertices.size(); i++,k++)
	{
		vertices[k].Pos = ball.Vertices[i].Position;
		vertices[k].Color = XMFLOAT4(DirectX::Colors::Red);
	}

	indices.insert(indices.end(), std::begin(cylinder.GetIndices_16()), std::end(cylinder.GetIndices_16()));
	indices.insert(indices.end(), std::begin(ball.GetIndices_16()), std::end(ball.GetIndices_16()));

	const UINT VertexBufferByteSize = (UINT)vertices.size() * sizeof(VertexInfo);
	const UINT IndexBufferByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto Geo = std::make_unique<MeshGeometry>();
	Geo->Name = "Geo";

	ThrowIfFailed(D3DCreateBlob(VertexBufferByteSize, &Geo->VertexBufferCPU))
	CopyMemory(Geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), VertexBufferByteSize);
	ThrowIfFailed(D3DCreateBlob(IndexBufferByteSize, &Geo->IndexBufferCPU))
	CopyMemory(Geo->IndexBufferCPU->GetBufferPointer(),indices.data(), IndexBufferByteSize);

	Geo->VertexBufferGPU = DXBase::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(), vertices.data(), VertexBufferByteSize, Geo->VertexBufferUploader);
	Geo->IndexBufferGPU = DXBase::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(), indices.data(), IndexBufferByteSize, Geo->IndexBufferUploader);

	Geo->VertexByteStride = sizeof(VertexInfo);
	Geo->VertexBufferByteSize = VertexBufferByteSize;
	Geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	Geo->IndexBufferByteSize = IndexBufferByteSize;

	Geo->SubmeshList[Geo_Cylinder.name] = Geo_Cylinder;
	Geo->SubmeshList[Geo_Ball.name] = Geo_Ball;

	mGeos[Geo->Name] = std::move(Geo);
}
//创建材质
void MyApp::BuildMaterials()
{
	UINT MaterialIndex = 0;

	auto grass = std::make_unique<Material>();
	grass->name = "Grass";
	grass->MaterialConstBufferIndex = MaterialIndex++;
	grass->NumDirtyFrames = gNumFrameResources;
	grass->DiffuseAlbedo = XMFLOAT4(0.2f, 0.6f, 0.2f, 1.0f);
	grass->FresneRf0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	grass->Roughness = 0.125f;

	mMaterials[grass->name] = std::move(grass);
}
//创建渲染项
void MyApp::BuildRenderItems()
{
	auto leftCylinderRenderItem = std::make_unique<RenderItem>();
	auto leftBallRenderItem = std::make_unique<RenderItem>();
	auto rightCylinderRenderItem = std::make_unique<RenderItem>();
	auto rightBallRenderItem = std::make_unique<RenderItem>();

	UINT GeoObjectIndex = 0;

	XMMATRIX leftCylinderWorld = XMMatrixTranslation(0.0f, +0.0f, -2.0f);
	XMMATRIX leftBallWorld = XMMatrixTranslation(0.0f, +2.96f, -2.0f);
	XMMATRIX rightCylinderWorld = XMMatrixTranslation(0.0f, +0.0f ,+ 2.0f );
	XMMATRIX rightBallWorld = XMMatrixTranslation(0.0f, +2.96f, +2.0f);

	XMStoreFloat4x4(&leftCylinderRenderItem->World, leftCylinderWorld);
	leftCylinderRenderItem->ObjectConstBufferIndex = GeoObjectIndex++;
	leftCylinderRenderItem->Mat = mMaterials["Grass"].get();
	leftCylinderRenderItem->Geo = mGeos["Geo"].get();
	leftCylinderRenderItem->PrimitiveType = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	leftCylinderRenderItem->IndexCount = leftCylinderRenderItem->Geo->SubmeshList["Geo_Cylinder"].IndexCount;
	leftCylinderRenderItem->IndexStartLocation = leftCylinderRenderItem->Geo->SubmeshList["Geo_Cylinder"].IndexStartLocation;
	leftCylinderRenderItem->VertexBaseLocation = leftCylinderRenderItem->Geo->SubmeshList["Geo_Cylinder"].VertexBaseLocation;

	XMStoreFloat4x4(&leftBallRenderItem->World, leftBallWorld);
	leftBallRenderItem->ObjectConstBufferIndex = GeoObjectIndex++;
	leftBallRenderItem->Mat = mMaterials["Grass"].get();
	leftBallRenderItem->Geo = mGeos["Geo"].get();
	leftBallRenderItem->PrimitiveType = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	leftBallRenderItem->IndexCount = leftCylinderRenderItem->Geo->SubmeshList["Geo_Ball"].IndexCount;
	leftBallRenderItem->IndexStartLocation = leftCylinderRenderItem->Geo->SubmeshList["Geo_Ball"].IndexStartLocation;
	leftBallRenderItem->VertexBaseLocation = leftCylinderRenderItem->Geo->SubmeshList["Geo_Ball"].VertexBaseLocation;

	XMStoreFloat4x4(&rightCylinderRenderItem->World, rightCylinderWorld);
	rightCylinderRenderItem->ObjectConstBufferIndex = GeoObjectIndex++;
	rightCylinderRenderItem->Mat = mMaterials["Grass"].get();
	rightCylinderRenderItem->Geo = mGeos["Geo"].get();
	rightCylinderRenderItem->PrimitiveType = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	rightCylinderRenderItem->IndexCount = leftCylinderRenderItem->Geo->SubmeshList["Geo_Cylinder"].IndexCount;
	rightCylinderRenderItem->IndexStartLocation = leftCylinderRenderItem->Geo->SubmeshList["Geo_Cylinder"].IndexStartLocation;
	rightCylinderRenderItem->VertexBaseLocation = leftCylinderRenderItem->Geo->SubmeshList["Geo_Cylinder"].VertexBaseLocation;

	XMStoreFloat4x4(&rightBallRenderItem->World, rightBallWorld);
	rightBallRenderItem->ObjectConstBufferIndex = GeoObjectIndex++;
	rightBallRenderItem->Mat = mMaterials["Grass"].get();
	rightBallRenderItem->Geo = mGeos["Geo"].get();
	rightBallRenderItem->PrimitiveType = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	rightBallRenderItem->IndexCount = leftCylinderRenderItem->Geo->SubmeshList["Geo_Ball"].IndexCount;
	rightBallRenderItem->IndexStartLocation = leftCylinderRenderItem->Geo->SubmeshList["Geo_Ball"].IndexStartLocation;
	rightBallRenderItem->VertexBaseLocation = leftCylinderRenderItem->Geo->SubmeshList["Geo_Ball"].VertexBaseLocation;

	mAllRenderItems.push_back(std::move(leftCylinderRenderItem));
	mAllRenderItems.push_back(std::move(leftBallRenderItem));
	mAllRenderItems.push_back(std::move(rightCylinderRenderItem));
	mAllRenderItems.push_back(std::move(rightBallRenderItem));

	for (auto& i: mAllRenderItems)
		mOpaqueRenderItems.push_back(i.get());
}
//创建帧资源
void MyApp::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; i++)
		mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(), 1, (UINT)mAllRenderItems.size(), (UINT)mMaterials.size()));
}
//创建程序所需的其他描述符堆（除初始化时创建的DSV、RTV描述符堆）
void MyApp::BuildDescriptorHeaps()
{
	mPassCbvOffset = (UINT)mOpaqueRenderItems.size() * gNumFrameResources;

	D3D12_DESCRIPTOR_HEAP_DESC CBV_HEAP_DESC;
	CBV_HEAP_DESC.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	CBV_HEAP_DESC.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	CBV_HEAP_DESC.NumDescriptors = ((UINT)mOpaqueRenderItems.size() + 1) * gNumFrameResources;
	CBV_HEAP_DESC.NodeMask = 0;

	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&CBV_HEAP_DESC, IID_PPV_ARGS(&mCbvDescriptorHeap)))
}
//创建常量缓冲区
void MyApp::BuildConstantBufferViews()
{
	UINT objConstantsBufferByteSize = DXBase::ConstUploadBufferByteSize256Alignment(sizeof(ObjectConstants));
	UINT passConstantsBufferByteSize = DXBase::ConstUploadBufferByteSize256Alignment(sizeof(RenderingPassConstants));
	for (UINT frameresourceIndex = 0; frameresourceIndex < gNumFrameResources; frameresourceIndex++)
	{
		//为物体常量缓冲区分配CBV描述符
		auto objectConstBuffer = mFrameResources[frameresourceIndex]->ObjectConstBuffer->Resource();
		for (UINT objectIndex = 0; objectIndex < (UINT)mOpaqueRenderItems.size(); objectIndex++)
		{
			D3D12_GPU_VIRTUAL_ADDRESS objConstantsBufferAddress = objectConstBuffer->GetGPUVirtualAddress();
			objConstantsBufferAddress += objectIndex * objConstantsBufferByteSize;

			int descriptorIndex = frameresourceIndex * mOpaqueRenderItems.size() + objectIndex;
			auto cbvCPUHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
			cbvCPUHandle.Offset(descriptorIndex, mCBV_SRV_UAVDescriptorSize);
			D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
			CBVDesc.BufferLocation = objConstantsBufferAddress;
			CBVDesc.SizeInBytes = objConstantsBufferByteSize;
			md3dDevice->CreateConstantBufferView(&CBVDesc, cbvCPUHandle);
		}
		//为渲染过程常量缓冲区分配CBV描述符
		auto passConstBuffer = mFrameResources[frameresourceIndex]->PassConstBuffer->Resource();

		D3D12_GPU_VIRTUAL_ADDRESS passConstantsBufferAddress = passConstBuffer->GetGPUVirtualAddress();

		UINT descriptorIndex = mPassCbvOffset + frameresourceIndex;
		auto cbvCPUHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
		cbvCPUHandle.Offset(descriptorIndex, mCBV_SRV_UAVDescriptorSize);
		D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
		CBVDesc.BufferLocation = passConstantsBufferAddress;
		CBVDesc.SizeInBytes = passConstantsBufferByteSize;
		md3dDevice->CreateConstantBufferView(&CBVDesc, cbvCPUHandle);
	}
}
//创建渲染管线状态对象
void MyApp::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC OpaquePSODesc;
	ZeroMemory(&OpaquePSODesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	OpaquePSODesc.pRootSignature = mRootSignature.Get();
	OpaquePSODesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["VS"]->GetBufferPointer()),
		mShaders["VS"]->GetBufferSize()
	};
	OpaquePSODesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["PS"]->GetBufferPointer()),
		mShaders["PS"]->GetBufferSize()
	};
	OpaquePSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	OpaquePSODesc.SampleMask = UINT_MAX;
	CD3DX12_RASTERIZER_DESC drd(D3D12_DEFAULT);
	drd.FillMode = D3D12_FILL_MODE_SOLID;
	drd.CullMode = D3D12_CULL_MODE_BACK;
	OpaquePSODesc.RasterizerState = drd;

	OpaquePSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	OpaquePSODesc.InputLayout = { mInputLayout.data(),(UINT)mInputLayout.size() };
	OpaquePSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	OpaquePSODesc.NumRenderTargets = 1;
	OpaquePSODesc.RTVFormats[0] = mBackBufferFormat;
	OpaquePSODesc.DSVFormat = mDepthStencilFormat;
	OpaquePSODesc.SampleDesc.Count = m4xMSAAState ? 4 : 1;
	OpaquePSODesc.SampleDesc.Quality = m4xMSAAState ? (m4xMSAAQuality - 1) : 0;
	OpaquePSODesc.NodeMask = 0;
	OpaquePSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&OpaquePSODesc, IID_PPV_ARGS(&mPSOs["Solid"])))

		D3D12_GRAPHICS_PIPELINE_STATE_DESC WireframePSODesc = OpaquePSODesc;
	drd.FillMode = D3D12_FILL_MODE_WIREFRAME;
	WireframePSODesc.RasterizerState = drd;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&WireframePSODesc, IID_PPV_ARGS(&mPSOs["Wireframe"])))
}

//改变窗口的高度和宽度
void MyApp::ChangeW_H(int width, int height)
{
	mClientWidth = width;
	mClientHeight = height;
}
//更改PSO
void MyApp::ChangePSOstate()
{
	if (GetAsyncKeyState('1') & 0x8000)
		mIsWireframe = true;
	else
		mIsWireframe = false;
}
//更新摄像头矩阵
void MyApp::UpdateCamara()
{
	mEyePos.x = mRadius * sinf(mPhi) * cosf(mTheta);
	mEyePos.z = mRadius * sinf(mPhi) * sinf(mTheta);
	mEyePos.y = mRadius * cosf(mPhi);

	XMVECTOR pos = XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up;
	if (sinf(mPhi) >= 0)
		up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	else
		up = XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);
}
//更新物体常量缓冲区（世界矩阵）
void MyApp::UpdateObjectsConstBuffers()const
{
	auto currentObjectConstBuffer = mCurrentFrameResource->ObjectConstBuffer.get();

	for (auto& it : mAllRenderItems)
	{
		if (it->NumDirtyFrames > 0)
		{
			XMMATRIX world = XMLoadFloat4x4(&it->World);
			ObjectConstants objectconstant;
			XMStoreFloat4x4(&objectconstant.XMWorld,  XMMatrixTranspose(world));

			currentObjectConstBuffer->CopyData(it->ObjectConstBufferIndex, objectconstant);

			it->NumDirtyFrames--;
		}
	}
}
//更新渲染过程常量
void MyApp::UpdatePassConstBuffers()const
{
	RenderingPassConstants mRenderingPassConstantsBuffer;

	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX viewProj=XMMatrixMultiply(view,proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&mRenderingPassConstantsBuffer.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mRenderingPassConstantsBuffer.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mRenderingPassConstantsBuffer.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mRenderingPassConstantsBuffer.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mRenderingPassConstantsBuffer.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mRenderingPassConstantsBuffer.InvViewProj, XMMatrixTranspose(invViewProj));

	mRenderingPassConstantsBuffer.EyePosW = mEyePos;
	mRenderingPassConstantsBuffer.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mRenderingPassConstantsBuffer.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mRenderingPassConstantsBuffer.NearZ = 1.0f;
	mRenderingPassConstantsBuffer.FarZ = 1000.0f;
	mRenderingPassConstantsBuffer.TotalTime = mGameTimer.TotalTime();
	mRenderingPassConstantsBuffer.DeltaTime = mGameTimer.DeltaTime();

	auto currentPassConstsBuffer = mCurrentFrameResource->PassConstBuffer.get();
	currentPassConstsBuffer->CopyData(0, mRenderingPassConstantsBuffer);
}
//更新材质常量缓冲区
void MyApp::UpdateMaterialConstBuffers()const
{
	auto currentMaterialConstBuffer = mCurrentFrameResource->MaterialConstBuffer.get();

	for (auto& ma : mMaterials)
	{
		Material* pMa = ma.second.get();
		if(pMa->NumDirtyFrames > 0)
		{
			MaterialConstants materialConstant;
			materialConstant.DiffuseAlbedo = pMa->DiffuseAlbedo;
			materialConstant.FresneRf0 = pMa->FresneRf0;
			materialConstant.Roughness = pMa->Roughness;
			materialConstant.MaterialTransform = pMa->MaterialTransform;
			currentMaterialConstBuffer->CopyData(pMa->MaterialConstBufferIndex, materialConstant);

			pMa->NumDirtyFrames--;
		}
	}
}

//绘制渲染项
void MyApp::DrawRenderItems(ID3D12GraphicsCommandList* commandList, const std::vector<RenderItem*>& renderItems)const
{
	for (size_t itemIndex = 0; itemIndex < renderItems.size(); itemIndex++)
	{
		auto item = renderItems[itemIndex];
		commandList->IASetVertexBuffers(0, 1, &item->Geo->VertexBufferView());
		commandList->IASetIndexBuffer(&item->Geo->IndexBufferView());
		commandList->IASetPrimitiveTopology(item->PrimitiveType);

		UINT cbvIndex = mCurrentFrameResourceIndex * renderItems.size() + item->ObjectConstBufferIndex;
		auto cbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		cbvHandle.Offset(cbvIndex, mCBV_SRV_UAVDescriptorSize);
		commandList->SetGraphicsRootDescriptorTable(0, cbvHandle);
		commandList->DrawIndexedInstanced(item->IndexCount, 1, item->IndexStartLocation, item->VertexBaseLocation, 0);
	}
}

//获取指向MyApp类自身的指针
const MyApp* MyApp::GetMyApp()const
{
	return this;
}