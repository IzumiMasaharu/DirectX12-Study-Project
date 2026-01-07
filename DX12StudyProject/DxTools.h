#pragma once

#include <DirectXMath.h>
#include <cstdlib>

// 数学帮手，给我这种脑残用的
class MathHelper
{
public:
    // 生成一个位于[0, 1)区间的随机float
    static float RandF()
    {
        return (float)(rand()) / (float)RAND_MAX;
    }
    // 生成[a, b)区间的随机float
    static float RandF(float a, float b)
    {
        return a + RandF() * (b - a);
    }
    // 生成[a, b)区间的随机int
    static int Rand(int a, int b)
    {
        return a + rand() % ((b - a) + 1);
    }
    // 返回两数之中较小值
    template<typename T>
    static T Min(const T& a, const T& b)
    {
        return a < b ? a : b;
    }
    // 返回两数之中较大值
    template<typename T>
    static T Max(const T& a, const T& b)
    {
        return a > b ? a : b;
    }
    // 返回a + (b - a) * t
    template<typename T>
    static T Lerp(const T& a, const T& b, float t)
    {
        return a + (b - a) * t;
    }
    // 用于限制x范围，即：若x小于low则返回low，若大于high则返回high，否则返回x本身
    template<typename T>
    static T Clamp(const T& x, const T& low, const T& high)
    {
        return x < low ? low : (x > high ? high : x);
    }

    // 将极坐标转换为直角坐标
    static DirectX::XMVECTOR SphericalToCartesian(float radius, float theta, float phi);
    // 返回M的逆矩阵的转置矩阵
    static DirectX::XMMATRIX InverseTranspose(DirectX::CXMMATRIX M);
    // 初始化4x4数组为单位数组
    static DirectX::XMFLOAT4X4 Identity4x4();
    // 返回直角坐标下（x，y）在极坐标下的极角
    static float AngleFromXY(float x, float y);
    // 生成一个随机的单位向量
    static DirectX::XMVECTOR RandUnitVec3();
    // 生成一个随机的单位向量，并且该向量位于给定向量 n 所在的半球内
    static DirectX::XMVECTOR RandHemisphereUnitVec3(DirectX::XMVECTOR n);
public:
    static const float Infinity; // 浮点数最大值
    static const float Pi;
};