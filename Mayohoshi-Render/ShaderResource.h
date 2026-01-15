#pragma once
#define MAX_NUM_LIGHTS 256
#define MAX_BINDING_TEXTURE 8

#include "DxTools.h"
#include "Light.h"

#include <array>
#include <Windows.h>

// 与着色器绑定的世界视图投影矩阵
struct InstanceData
{
    DirectX::XMFLOAT4X4 worldTransform = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 textureTransform = MathHelper::Identity4x4();

    UINT materialIndex;        // 对应 materialBuffer
    UINT textureTableIndex;    // 指向 gTextureTables
    UINT textureFlags;
    UINT padding;              // 16字节对齐
};

// 与着色器绑定的材质结构体
struct MaterialData
{
    DirectX::XMFLOAT4   albedo = { 1,1,1,1 };
    DirectX::XMFLOAT3   fresnel;
    float               roughness = 0.0f;
    DirectX::XMFLOAT3   emissive = { 0,0,0 };
    float               padding0;
    DirectX::XMFLOAT4X4 materialTransform = MathHelper::Identity4x4();
};

// 与着色器绑定的纹理表结构体
struct TextureTable
{
    UINT diffuseIndex;
    UINT normalIndex;
    UINT depthIndex;
    UINT padding0;
};

// 与着色器绑定的渲染过程常量结构体
struct RenderingPassConstants
{
    DirectX::XMFLOAT4X4 view = MathHelper::Identity4x4();               // 摄像机视图矩阵
    DirectX::XMFLOAT4X4 invView = MathHelper::Identity4x4();            // 视图矩阵的逆矩阵
    DirectX::XMFLOAT4X4 proj = MathHelper::Identity4x4();               // 投影（至显示屏幕）矩阵
    DirectX::XMFLOAT4X4 invProj = MathHelper::Identity4x4();            // 投影矩阵的逆矩阵
    DirectX::XMFLOAT4X4 viewProj = MathHelper::Identity4x4();           // 视图投影矩阵
    DirectX::XMFLOAT4X4 invViewProj = MathHelper::Identity4x4();        // 视图投影矩阵的逆矩阵
    DirectX::XMFLOAT3 eyePosW = { 0.0f,0.0f,0.0f };                     // 摄像机位置坐标
    float cbPerObjectPad1 = 0.0f;	                                    
    DirectX::XMFLOAT2 renderTargetSize = { 1.0f,1.0f };                 // 渲染目标的大小
    DirectX::XMFLOAT2 invRenderTargetSize = { 1.0f,1.0f };              // 渲染目标大小的倒数
    float nearZ = 0.0f;                                                 // 近视平面
    float farZ = 0.0f;                                                  // 远视平面
    float totalTime = 0;                                                // 程序运行总时间
    float deltaTime = 0;                                                // 两次tick之间的时间差
    DirectX::XMFLOAT4 ambientIlluminating = { 0.0f,0.0f,0.0f,1.0f };    // 环境光项

    std::array<Light, MAX_NUM_LIGHTS> lights = {};                      // 场景中的光源数组
};

// 输入到渲染管线的顶点结构体
struct VertexConstants
{
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT3 tangent;
    DirectX::XMFLOAT2 textureUV;
};