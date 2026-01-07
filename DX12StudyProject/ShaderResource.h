#pragma once
#define MAX_NUM_LIGHTS 256
#define MAX_BINDING_TEXTURE 8

#include "DxTools.h"
#include "Light.h"

#include <array>
#include <Windows.h>

// 与着色器绑定的世界视图投影矩阵
struct ObjectConstants
{
    DirectX::XMFLOAT4X4 worldTransform = MathHelper::Identity4x4(); // 物体的世界变换矩阵
    DirectX::XMFLOAT4X4 normalTransform = MathHelper::Identity4x4(); // 法线变换矩阵
    DirectX::XMFLOAT4X4 textureTransform = MathHelper::Identity4x4(); // 纹理变换矩阵 

    std::array<UINT, MAX_BINDING_TEXTURE> diffuseTextureIndex{};
    std::array<UINT, MAX_BINDING_TEXTURE> normalTextureIndex{};
    std::array<UINT, MAX_BINDING_TEXTURE> depthTextureIndex{};
    UINT materialIndex; // 该渲染项所使用的材质
    UINT textureFlags;
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
    DirectX::XMFLOAT4 ambientIlluminating = { 0.0f,0.0f,0.0f,1.0f }; // 环境光项

    std::array<Light, MAX_NUM_LIGHTS> lights = {}; // 场景中的光源数组
};

// 输入到渲染管线的顶点结构体
struct VertexConstants
{
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT3 tangent;
    DirectX::XMFLOAT2 textureUV;
};