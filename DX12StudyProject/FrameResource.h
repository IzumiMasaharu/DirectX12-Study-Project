#pragma once
#include "DXBase.h"
#include "UploadBuffer.h"

//与着色器绑定的顶点结构体
struct VertexInfo
{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT4 Color;
};

//与着色器绑定的材质结构体
struct MaterialConstants
{
    DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f,1.0f,1.0f,1.0f };//漫反射反照率
    DirectX::XMFLOAT3 FresneRf0 = { 0.0f,0.0f,0.0f };//菲涅尔效应材质属性Rf（0°）
    float Roughness = 0.0f;//材质粗糙度

    DirectX::XMFLOAT4X4 MaterialTransform = MathHelper::Identity4x4();
};
//与着色器绑定的世界视图投影矩阵（物体常量缓冲区）
struct ObjectConstants
{
    DirectX::XMFLOAT4X4 XMWorld = MathHelper::Identity4x4();//物体的世界变换矩阵
};
//与着色器绑定的渲染过程常量结构体
struct RenderingPassConstants
{
    DirectX::XMFLOAT4X4 View = MathHelper::Identity4x4();//摄像机视图矩阵
    DirectX::XMFLOAT4X4 InvView = MathHelper::Identity4x4();//视图矩阵的逆矩阵
    DirectX::XMFLOAT4X4 Proj = MathHelper::Identity4x4();//投影（至显示屏幕）矩阵
    DirectX::XMFLOAT4X4 InvProj = MathHelper::Identity4x4();//投影矩阵的逆矩阵
    DirectX::XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();//视图投影矩阵
    DirectX::XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();//视图投影矩阵的逆矩阵
    DirectX::XMFLOAT3 EyePosW = { 0.0f,0.0f,0.0f };//摄像机位置坐标
    float cbPerObjectPad1 = 0.0f;//
    DirectX::XMFLOAT2 RenderTargetSize = { 1.0f,1.0f };//渲染目标的大小
    DirectX::XMFLOAT2 InvRenderTargetSize = { 1.0f,1.0f };//渲染目标大小的倒数
    float NearZ = 0.0f;//近视平面
    float FarZ = 0.0f;//远视平面
    float TotalTime = 0;//程序运行总时间
    float DeltaTime = 0;//两次tick之间的时间差
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
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandAllocator;//每个帧资源的命令分配器
    std::unique_ptr<UploadBuffer<RenderingPassConstants>> PassConstBuffer = nullptr;//每个帧资源的渲染过程常量缓冲区
    std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectConstBuffer=nullptr;//每个帧资源的物体常量缓冲区
    std::unique_ptr<UploadBuffer<MaterialConstants>> MaterialConstBuffer = nullptr;//每个帧资源的材质常量缓冲区
    UINT64 Fence = 0;
};

