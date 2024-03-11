#pragma once
#define MAX_NUM_LIGHTS 8
#include "DXBase.h"
#include "UploadBuffer.h"

//与着色器绑定的顶点结构体
struct VertexConstants
{
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT3 normal;
};

//与着色器绑定的材质结构体
struct MaterialConstants
{
    DirectX::XMFLOAT4 diffuseAlbedo = { 1.0f,1.0f,1.0f,1.0f };//漫反射反照率
    DirectX::XMFLOAT3 fresneRf0 = { 0.0f,0.0f,0.0f };//菲涅尔效应材质属性Rf（0°）
    float roughness = 0.0f;//材质粗糙度
    DirectX::XMFLOAT4X4 materialTransform = MathHelper::Identity4x4();
};
//与着色器绑定的世界视图投影矩阵（物体常量缓冲区）
struct ObjectConstants
{
    DirectX::XMFLOAT4X4 XMWorld = MathHelper::Identity4x4();//物体的世界变换矩阵
};
//与着色器绑定的渲染过程常量结构体
struct RenderingPassConstants
{
    DirectX::XMFLOAT4X4 view = MathHelper::Identity4x4();//摄像机视图矩阵
    DirectX::XMFLOAT4X4 invView = MathHelper::Identity4x4();//视图矩阵的逆矩阵
    DirectX::XMFLOAT4X4 proj = MathHelper::Identity4x4();//投影（至显示屏幕）矩阵
    DirectX::XMFLOAT4X4 invProj = MathHelper::Identity4x4();//投影矩阵的逆矩阵
    DirectX::XMFLOAT4X4 viewProj = MathHelper::Identity4x4();//视图投影矩阵
    DirectX::XMFLOAT4X4 invViewProj = MathHelper::Identity4x4();//视图投影矩阵的逆矩阵
    DirectX::XMFLOAT3 eyePosW = { 0.0f,0.0f,0.0f };//摄像机位置坐标
    float cbPerObjectPad1 = 0.0f;//
    DirectX::XMFLOAT2 renderTargetSize = { 1.0f,1.0f };//渲染目标的大小
    DirectX::XMFLOAT2 invRenderTargetSize = { 1.0f,1.0f };//渲染目标大小的倒数
    float nearZ = 0.0f;//近视平面
    float farZ = 0.0f;//远视平面
    float totalTime = 0;//程序运行总时间
    float deltaTime = 0;//两次tick之间的时间差
    DirectX::XMFLOAT4 ambientIlluminating= { 0.0f,0.0f,0.0f,1.0f };//物体自身发光

    Light lights[MAX_NUM_LIGHTS];
};

//帧资源
struct FrameResource
{
public:
    FrameResource(ID3D12Device* device, UINT PassCount, UINT ObjectCount, UINT MaterialCount);
    FrameResource(const FrameResource& rhs) = delete;
    ~FrameResource() = default;
public:
    FrameResource& operator=(const FrameResource& rhs) = delete;
public:
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;//每个帧资源的命令分配器
    std::unique_ptr<UploadBuffer<RenderingPassConstants>> passConstBuffer = nullptr;//每个帧资源的渲染过程常量缓冲区
    std::unique_ptr<UploadBuffer<ObjectConstants>> objectConstBuffer=nullptr;//每个帧资源的物体常量缓冲区
    std::unique_ptr<UploadBuffer<MaterialConstants>> materialConstBuffer = nullptr;//每个帧资源的材质常量缓冲区
    UINT64 fence = 0;
};

