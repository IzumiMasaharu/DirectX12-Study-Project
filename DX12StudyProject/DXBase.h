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
//��stringתΪwstring
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
    std::wstring ErrorMessageString()const;//��ȡ������Ϣ������������Ϣת��Ϊ��������ַ���
public:
    HRESULT errorCode = S_OK;
    std::wstring functionName;
    std::wstring fileName;
    UINT lineNum = -1;
};
class DXBase
{
public:
    //���ʣ�Ϊ��Ҫ��UploadBuffer��Ϊ�������ݽ����� �������ں����д���һ��ComPtr<ID3D12Resource> UploadBuffer�����Դ���ݲ���?
    //��UploadBuffer�����������٣���Ϊ�����б��Ʋ�����CreateDefaultBuffer�����������ʱ������δִ�У�����ȵ����ߵ�֪������ɺ�����ͷ�UploadBuffer��
    static Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        const void* initData,
        UINT64 byteSize,
        Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);
    //�����ݴ�С�ֽڶ���Ϊ256b�����䳣��������
    static UINT ConstUploadBufferByteSize256Alignment(UINT ByteSize); 
    //���߱�����ɫ��
    static Microsoft::WRL::ComPtr<ID3DBlob> CompileShaderOnline(
        const std::wstring& hlsl_filename,
        const D3D_SHADER_MACRO* defines,
        const std::string& Entrypoint,
        const std::string& TargetShaderType);
    //���������ַ���д��Blob�ļ�
    static Microsoft::WRL::ComPtr<ID3DBlob> LoadBinaryToBlob(const std::wstring& Binary_filename);
};
//��ѧ���֣����������Բ��õ�
class MathHelper
{
public:
    // ����һ��λ��[0, 1)��������float
    static float RandF()
    {
        return (float)(rand()) / (float)RAND_MAX;
    }
    // ����[a, b)��������float
    static float RandF(float a, float b)
    {
        return a + RandF() * (b - a);
    }
    //����[a, b)��������int
    static int Rand(int a, int b)
    {
        return a + rand() % ((b - a) + 1);
    }
    //��������֮�н�Сֵ
    template<typename T>
    static T Min(const T& a, const T& b)
    {
        return a < b ? a : b;
    }
    //��������֮�нϴ�ֵ
    template<typename T>
    static T Max(const T& a, const T& b)
    {
        return a > b ? a : b;
    }
    //����a + (b - a) * t
    template<typename T>
    static T Lerp(const T& a, const T& b, float t)
    {
        return a + (b - a) * t;
    }
    //��������x��Χ��������xС��low�򷵻�low��������high�򷵻�high�����򷵻�x����
    template<typename T>
    static T Clamp(const T& x, const T& low, const T& high)
    {
        return x < low ? low : (x > high ? high : x);
    }

    //��������ת��Ϊֱ������
    static DirectX::XMVECTOR SphericalToCartesian(float radius, float theta, float phi);
    //����M��������ת�þ���
    static DirectX::XMMATRIX InverseTranspose(DirectX::CXMMATRIX M);
    //��ʼ��4x4����Ϊ��λ����
    static DirectX::XMFLOAT4X4 Identity4x4();
    // ����ֱ�������£�x��y���ڼ������µļ���
    static float AngleFromXY(float x, float y);

    static DirectX::XMVECTOR RandUnitVec3();
    static DirectX::XMVECTOR RandHemisphereUnitVec3(DirectX::XMVECTOR n);
public:
    static const float Infinity;//���������ֵ
    static const float Pi;
};

//�洢����������Ľṹ��
struct SubmeshGeometry
{
    std::string name;

    UINT indexCount = 0;//������Ŀ
    UINT indexStartLocation = 0;//������ʼλ��
    UINT vertexBaseLocation = 0;//��׼����λ��
    DirectX::BoundingBox bounds;//���ñ߽��
};
//�洢ȫ��������Ľṹ��
struct MeshGeometry
{
public:
    //��GPU�еĶ��㻺������Դװ�سɶ��㻺������ͼ��װ�䵽Pipeline IA�׶�
    D3D12_VERTEX_BUFFER_VIEW VertexBufferView()const
    {
        D3D12_VERTEX_BUFFER_VIEW vbv;
        vbv.BufferLocation = vertexBufferGPU->GetGPUVirtualAddress();
        vbv.StrideInBytes = vertexByteStride;
        vbv.SizeInBytes = vertexBufferByteSize;

        return vbv;
    }
    //��GPU�е�������������Դװ�س�������������ͼ��װ�䵽Pipeline IA�׶�
    D3D12_INDEX_BUFFER_VIEW IndexBufferView()const
    {
        D3D12_INDEX_BUFFER_VIEW ibv;
        ibv.BufferLocation = indexBufferGPU->GetGPUVirtualAddress();
        ibv.Format = indexFormat;
        ibv.SizeInBytes = indexBufferByteSize;

        return ibv;
    }
    // ����GPU�ϴ�����Դ֮���ͷŶ����ϴ��������������ϴ�������
    void DisposeUploaders()
    {
        vertexBufferUploader = nullptr;
        indexBufferUploader = nullptr;
    }
public:
    std::string name;

    Microsoft::WRL::ComPtr<ID3DBlob> vertexBufferCPU = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> indexBufferCPU = nullptr;
    //                       ��
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBufferUploader = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexBufferUploader = nullptr;
    //                       ��
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBufferGPU = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexBufferGPU = nullptr;

    UINT vertexByteStride = 0;//����ṹ���С
    UINT vertexBufferByteSize = 0;//���㻺������С
    DXGI_FORMAT indexFormat = DXGI_FORMAT_R16_UINT;//��Դ��ʽ
    UINT indexBufferByteSize = 0;//������������С

    std::unordered_map<std::string, SubmeshGeometry> submeshList;//��ȫ����������Ϊ���ɵ��������壬�洢������ͼ��
};

//�洢�������ݵĽṹ��
struct Material
{
    std::string name;

    UINT materialConstBufferIndex = -1;//�ò����ڳ����������е�����
    UINT diffuseSrvHeapIndex = -1;//������������SRV���е�����
    UINT normalSrvHeapIndex = -1;
    UINT numDirtyFrames = -1;//�����µ�֡��Դ����

    DirectX::XMFLOAT4 diffuseAlbedo = { 1.0f,1.0f,1.0f,1.0f };//�����䷴����
    DirectX::XMFLOAT3 fresneRf0 = { 0.0f,0.0f,0.0f };//������ЧӦ��������Rf��0�㣩
    float roughness = 0.0f;//���ʴֲڶ�
    DirectX::XMFLOAT4X4 materialTransform = MathHelper::Identity4x4();
};
//����˳�򲻿ɸı䣬����hlsl��˳��һһ��Ӧ���Ա�֤shader�ܶԲ�����ȷ���Ϊ4D����
struct Light
{
    DirectX::XMFLOAT3 rgbIntensity = { 1.0f, 1.0f, 1.0f };//��Դ��RGBֵ
    float start = 0.0f;                                   //���Դ���۹��ʹ�ã�ָ����Դ�����䵽���������
    DirectX::XMFLOAT3 direction = { 0.0f, 0.0f, 1.0f };  //ƽ�й⡢�۹��ʹ�ã�ָ����Դ����
    float end = 10.0f;                                    //���Դ���۹��ʹ�ã�ָ����Դ�����䵽����Զ����
    DirectX::XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };    //���Դ���۹��ʹ�ã�ָ����Դλ��
    float spotPower = 128.0f;                              //�۹��ʹ��,
};
struct Texture
{
    std::string name;

    std::wstring filename;

    Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> uploadHeap = nullptr;
};

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                              \
{                                                                     \
    HRESULT hr__ = (x);                                               \
    std::wstring wfn = AnsiToWstring(__FILE__);                       \
    if(FAILED(hr__)) { throw DxException(hr__, L#x, wfn, __LINE__); } \
}
#endif
//�ͷŶ�ָ̬��
#ifndef ReleaseCom
#define ReleaseCom(x) { if(x){ x->Release(); x = 0; } }
#endif
