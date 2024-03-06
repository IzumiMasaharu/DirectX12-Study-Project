#pragma once
#include"DXBase.h"
#include"UploadBuffer.h"

//顶点结构体
struct VertexPos
{
	XMFLOAT3 Pos;
};
struct VertexColor
{
	XMFLOAT4 Color;
};
//世界视图投影矩阵
struct ObjectConstants
{
	XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4();
};
//渲染过程常量数据
struct RenderingPassConstants
{
    XMFLOAT4X4 View = MathHelper::Identity4x4();//摄像机视图矩阵
    XMFLOAT4X4 InvView = MathHelper::Identity4x4();//视图矩阵的逆矩阵
    XMFLOAT4X4 Proj = MathHelper::Identity4x4();//投影（至显示屏幕）矩阵
    XMFLOAT4X4 InvProj = MathHelper::Identity4x4();//投影矩阵的逆矩阵
    XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();//视图投影矩阵
    XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();//视图投影矩阵的逆矩阵
    XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };//摄像机位置坐标
    float cbPerObjectPad1 = 0.0f;//
    XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };//渲染目标的大小
    XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };//渲染目标大小的倒数
    float NearZ = 0.0f;//近视平面
    float FarZ = 0.0f;//远视平面
    float TotalTime = 0.0f;//程序运行总时间
    float DeltaTime = 0.0f;//两次tick之间的时间差
};

//帧资源
struct Framesource
{
public:
    Framesource(ID3D12Device* device, UINT PassCount, UINT ObjectCount);
    Framesource(const Framesource& rhs) = delete;
    Framesource& operator=(const Framesource& rhs) = delete;
    ~Framesource()=default;
};
