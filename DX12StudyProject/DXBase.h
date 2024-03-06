#pragma once

#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"D3D12.lib")
#pragma comment(lib,"dxgi.lib")

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <comdef.h>
#include <cstdint>
#include <DirectXCollision.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <d3d12.h>
#include <float.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <windowsx.h>
#include <Windows.h>
#include <wrl.h>
#include "d3dx12.h"
#include "d3d12.h"
#include "resource.h"

inline void d3dSetDebugName(IDXGIObject* obj, const char* name)
{
    if (obj)
    {
        obj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(name), name);
    }
}
inline void d3dSetDebugName(ID3D12Device* obj, const char* name)
{
    if (obj)
    {
        obj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(name), name);
    }
}
inline void d3dSetDebugName(ID3D12DeviceChild* obj, const char* name)
{
    if (obj)
    {
        obj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(name), name);
    }
}
//将string转为wstring
inline std::wstring AnsiToWstring(const std::string& str)
{
    WCHAR buffer[512];
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
    return std::wstring(buffer);
}

class DxException
{
public:
    DxException() = default;
    DxException(HRESULT hr, const std::wstring& function_name, const std::wstring& file_name, UINT line_num);
public:
    std::wstring ErrorMessageString()const;//读取错误信息，并将错误信息转化为可输出的字符串
public:
    HRESULT ErrorCode = S_OK;
    std::wstring FunctionName;
    std::wstring FileName;
    UINT LineNum = -1;
};
class DXBase
{
public:
    //疑问：为何要把UploadBuffer作为参数传递进函数 而不是在函数中创建一个ComPtr<ID3D12Resource> UploadBuffer完成资源传递操作?
    //答：UploadBuffer不能立即销毁，因为命令列表复制操作在CreateDefaultBuffer（）调用完毕时可能尚未执行，必须等调用者得知复制完成后才能释放UploadBuffer。
    static Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        const void* initData,
        UINT64 byteSize,
        Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);
    //将数据大小字节对齐为256b以适配常量缓冲区
    static UINT ConstUploadBufferByteSize256Alignment(UINT ByteSize); 
    //在线编译着色器
    static Microsoft::WRL::ComPtr<ID3DBlob> CompileShaderOnline(
        const std::wstring& hlsl_filename,
        const D3D_SHADER_MACRO* defines,
        const std::string& Entrypoint,
        const std::string& TargetShaderType);
    //将二进制字符串写进Blob文件
    static Microsoft::WRL::ComPtr<ID3DBlob> LoadBinaryToBlob(const std::wstring& Binary_filename);
};
//数学帮手，给我这种脑残用的
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
    //生成[a, b)区间的随机int
    static int Rand(int a, int b)
    {
        return a + rand() % ((b - a) + 1);
    }
    //返回两数之中较小值
    template<typename T>
    static T Min(const T& a, const T& b)
    {
        return a < b ? a : b;
    }
    //返回两数之中较大值
    template<typename T>
    static T Max(const T& a, const T& b)
    {
        return a > b ? a : b;
    }
    //返回a + (b - a) * t
    template<typename T>
    static T Lerp(const T& a, const T& b, float t)
    {
        return a + (b - a) * t;
    }
    //用于限制x范围，即：若x小于low则返回low，若大于high则返回high，否则返回x本身
    template<typename T>
    static T Clamp(const T& x, const T& low, const T& high)
    {
        return x < low ? low : (x > high ? high : x);
    }

    //将极坐标转换为直角坐标
    static DirectX::XMVECTOR SphericalToCartesian(float radius, float theta, float phi);
    //返回M的逆矩阵的转置矩阵
    static DirectX::XMMATRIX InverseTranspose(DirectX::CXMMATRIX M);
    //初始化4x4数组为单位数组
    static DirectX::XMFLOAT4X4 Identity4x4();
    // 返回直角坐标下（x，y）在极坐标下的极角
    static float AngleFromXY(float x, float y);

    static DirectX::XMVECTOR RandUnitVec3();
    static DirectX::XMVECTOR RandHemisphereUnitVec3(DirectX::XMVECTOR n);
public:
    static const float Infinity;//浮点数最大值
    static const float Pi;
};

//存储单个网格体的结构体
struct SubmeshGeometry
{
    std::string name;

    UINT IndexCount = 0;//索引数目
    UINT IndexStartLocation = 0;//索引开始位置
    UINT VertexBaseLocation = 0;//基准定点位置
    DirectX::BoundingBox Bounds;//设置边界框
};
//存储全部网格体的结构体
struct MeshGeometry
{
public:
    //将GPU中的顶点缓冲区资源装载成顶点缓冲区视图，装配到Pipeline IA阶段
    D3D12_VERTEX_BUFFER_VIEW VertexBufferView()const
    {
        D3D12_VERTEX_BUFFER_VIEW vbv;
        vbv.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
        vbv.StrideInBytes = VertexByteStride;
        vbv.SizeInBytes = VertexBufferByteSize;

        return vbv;
    }
    //将GPU中的索引缓冲区资源装载成索引缓冲区视图，装配到Pipeline IA阶段
    D3D12_INDEX_BUFFER_VIEW IndexBufferView()const
    {
        D3D12_INDEX_BUFFER_VIEW ibv;
        ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
        ibv.Format = IndexFormat;
        ibv.SizeInBytes = IndexBufferByteSize;

        return ibv;
    }
    // 在向GPU上传完资源之后释放顶点上传缓冲区和索引上传缓冲区
    void DisposeUploaders()
    {
        VertexBufferUploader = nullptr;
        IndexBufferUploader = nullptr;
    }
public:
    std::string Name;

    Microsoft::WRL::ComPtr<ID3DBlob> VertexBufferCPU = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> IndexBufferCPU = nullptr;
    //                       ↓
    Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;
    //                       ↓
    Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferGPU = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;

    UINT VertexByteStride = 0;//顶点结构体大小
    UINT VertexBufferByteSize = 0;//顶点缓冲区大小
    DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;//资源格式
    UINT IndexBufferByteSize = 0;//索引缓冲区大小

    std::unordered_map<std::string, SubmeshGeometry> SubmeshList;//将全部网格体拆分为若干单个网格体，存储到无序图中
};

//存储材质数据的结构体
struct Material
{
    std::string name;

    UINT MaterialConstBufferIndex = -1;//该材质在常量缓冲区中的索引
    UINT DiffuseSrvHeapIndex = -1;//漫反射纹理在SRV堆中的索引
    UINT NumDirtyFrames = -1;//待更新的帧资源数量

    DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f,1.0f,1.0f,1.0f };//漫反射反照率
    DirectX::XMFLOAT3 FresneRf0 = { 0.0f,0.0f,0.0f };//菲涅尔效应材质属性Rf（0°）
    float Roughness = 0.0f;//材质粗糙度
    DirectX::XMFLOAT4X4 MaterialTransform = MathHelper::Identity4x4();
};

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                              \
{                                                                     \
    HRESULT hr__ = (x);                                               \
    std::wstring wfn = AnsiToWstring(__FILE__);                       \
    if(FAILED(hr__)) { throw DxException(hr__, L#x, wfn, __LINE__); } \
}
#endif
//释放动态指针
#ifndef ReleaseCom
#define ReleaseCom(x) { if(x){ x->Release(); x = 0; } }
#endif
