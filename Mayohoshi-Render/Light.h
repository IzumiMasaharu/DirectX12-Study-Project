#pragma once
#include <DirectXMath.h>

// 参数顺序不可改变，且与hlsl中顺序一一对应，以保证shader能对参数正确打包为4D向量
struct Light
{
    DirectX::XMFLOAT3 rgbIntensity = { 1.0f, 1.0f, 1.0f }; // 光源的RGB值
    float start = 0.0f;                                    // 点光源、聚光灯使用，指定光源能照射到的最近距离
    DirectX::XMFLOAT3 direction = { 0.0f, 0.0f, 1.0f };    // 平行光、聚光灯使用，指定光源方向
    float end = 10.0f;                                     // 点光源、聚光灯使用，指定光源能照射到的最远距离
    DirectX::XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };     // 点光源、聚光灯使用，指定光源位置
    float spotPower = 128.0f;                              // 聚光灯使用,
};