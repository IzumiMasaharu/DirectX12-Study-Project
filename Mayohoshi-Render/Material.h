#pragma once
#include <string>
#include <Windows.h>
#include <DirectXMath.h>

// 存储材质数据的结构体
struct Material
{
    std::string name;

    UINT materialIndex = -1;    // 该材质在结构缓冲区中的索引
    UINT numDirtyFrames = -1;   // 待更新的帧资源数量

    DirectX::XMFLOAT4 albedo = { 1.0f,1.0f,1.0f,1.0f }; // 漫反射反照率
    float metallic = 0.0f;								// 金属度
    float roughness = 0.0f;                             // 材质粗糙度
    float ior = 1.0f;									// 折射率
    DirectX::XMFLOAT3 emissive = { 0,0,0 };             // 自发光
};