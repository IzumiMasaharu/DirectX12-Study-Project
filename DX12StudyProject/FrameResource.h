#pragma once
#define MAX_NUM_LIGHTS 8
#include "DXBase.h"
#include "UploadBuffer.h"

// 与着色器绑定的世界视图投影矩阵
struct ObjectConstants
{
    DirectX::XMFLOAT4X4 worldTransform = MathHelper::Identity4x4(); // 物体的世界变换矩阵
    DirectX::XMFLOAT4X4 normalTransform = MathHelper::Identity4x4(); // 法线变换矩阵
	DirectX::XMFLOAT4X4 textureTransform = MathHelper::Identity4x4(); // 纹理变换矩阵 
	UINT diffuseTextureIndex[MAX_BINDING_TEXTURE];
	UINT normalTextureIndex[MAX_BINDING_TEXTURE];
    UINT materialIndex; // 该渲染项所使用的材质
};

// 与着色器绑定的材质结构体
struct MaterialData
{
    DirectX::XMFLOAT4 albedo = { 1.0f,1.0f,1.0f,1.0f };	// 漫反射反照率
    DirectX::XMFLOAT3 fresnel;
    float roughness = 0.0f;										// 材质粗糙度
	DirectX::XMFLOAT3 emissive = { 0,0,0 };						// 自发光
};

// 与着色器绑定的渲染过程常量结构体
struct RenderingPassConstants
{
    DirectX::XMFLOAT4X4 view = MathHelper::Identity4x4(); // 摄像机视图矩阵
    DirectX::XMFLOAT4X4 invView = MathHelper::Identity4x4(); // 视图矩阵的逆矩阵
    DirectX::XMFLOAT4X4 proj = MathHelper::Identity4x4(); // 投影（至显示屏幕）矩阵
    DirectX::XMFLOAT4X4 invProj = MathHelper::Identity4x4(); // 投影矩阵的逆矩阵
    DirectX::XMFLOAT4X4 viewProj = MathHelper::Identity4x4(); // 视图投影矩阵
    DirectX::XMFLOAT4X4 invViewProj = MathHelper::Identity4x4(); // 视图投影矩阵的逆矩阵
    DirectX::XMFLOAT3 eyePosW = { 0.0f,0.0f,0.0f }; // 摄像机位置坐标
    float cbPerObjectPad1 = 0.0f;	// Padding
    DirectX::XMFLOAT2 renderTargetSize = { 1.0f,1.0f }; // 渲染目标的大小
    DirectX::XMFLOAT2 invRenderTargetSize = { 1.0f,1.0f }; // 渲染目标大小的倒数
    float nearZ = 0.0f; // 近视平面
    float farZ = 0.0f; // 远视平面
    float totalTime = 0; // 程序运行总时间
    float deltaTime = 0; // 两次tick之间的时间差
    DirectX::XMFLOAT4 ambientIlluminating= { 0.0f,0.0f,0.0f,1.0f }; // 环境光项

    Light lights[MAX_NUM_LIGHTS];
};

// 输入到渲染管线的顶点结构体
struct VertexConstants
{
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT3 tangent;
    DirectX::XMFLOAT2 textureUV;
};

// 帧资源
struct FrameResource
{
public:
    FrameResource(ID3D12Device* device, UINT PassCount, UINT ObjectCount, UINT MaterialCount);
    FrameResource(const FrameResource& rhs) = delete;
    ~FrameResource() = default;
public:
    FrameResource& operator=(const FrameResource& rhs) = delete;
public:
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator; // 每个帧资源的命令分配器
    std::unique_ptr<UploadBuffer<RenderingPassConstants>> passConstBuffer = nullptr; // 每个帧资源的渲染过程常量缓冲区
    std::unique_ptr<UploadBuffer<ObjectConstants>> objectConstBuffer = nullptr; // 每个帧资源的物体常量缓冲区

    std::unique_ptr<UploadBuffer<MaterialData>> materialStructuredBuffer = nullptr; // 结构化材质常量缓冲区
    UINT64 fence = 0;
};

