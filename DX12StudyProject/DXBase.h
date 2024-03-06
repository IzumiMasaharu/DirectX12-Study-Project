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
    HRESULT ErrorCode = S_OK;
    std::wstring FunctionName;
    std::wstring FileName;
    UINT LineNum = -1;
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

    UINT IndexCount = 0;//������Ŀ
    UINT IndexStartLocation = 0;//������ʼλ��
    UINT VertexBaseLocation = 0;//��׼����λ��
    DirectX::BoundingBox Bounds;//���ñ߽��
};
//�洢ȫ��������Ľṹ��
struct MeshGeometry
{
public:
    //��GPU�еĶ��㻺������Դװ�سɶ��㻺������ͼ��װ�䵽Pipeline IA�׶�
    D3D12_VERTEX_BUFFER_VIEW VertexBufferView()const
    {
        D3D12_VERTEX_BUFFER_VIEW vbv;
        vbv.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
        vbv.StrideInBytes = VertexByteStride;
        vbv.SizeInBytes = VertexBufferByteSize;

        return vbv;
    }
    //��GPU�е�������������Դװ�س�������������ͼ��װ�䵽Pipeline IA�׶�
    D3D12_INDEX_BUFFER_VIEW IndexBufferView()const
    {
        D3D12_INDEX_BUFFER_VIEW ibv;
        ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
        ibv.Format = IndexFormat;
        ibv.SizeInBytes = IndexBufferByteSize;

        return ibv;
    }
    // ����GPU�ϴ�����Դ֮���ͷŶ����ϴ��������������ϴ�������
    void DisposeUploaders()
    {
        VertexBufferUploader = nullptr;
        IndexBufferUploader = nullptr;
    }
public:
    std::string Name;

    Microsoft::WRL::ComPtr<ID3DBlob> VertexBufferCPU = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> IndexBufferCPU = nullptr;
    //                       ��
    Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;
    //                       ��
    Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferGPU = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;

    UINT VertexByteStride = 0;//����ṹ���С
    UINT VertexBufferByteSize = 0;//���㻺������С
    DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;//��Դ��ʽ
    UINT IndexBufferByteSize = 0;//������������С

    std::unordered_map<std::string, SubmeshGeometry> SubmeshList;//��ȫ����������Ϊ���ɵ��������壬�洢������ͼ��
};

//�洢�������ݵĽṹ��
struct Material
{
    std::string name;

    UINT MaterialConstBufferIndex = -1;//�ò����ڳ����������е�����
    UINT DiffuseSrvHeapIndex = -1;//������������SRV���е�����
    UINT NumDirtyFrames = -1;//�����µ�֡��Դ����

    DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f,1.0f,1.0f,1.0f };//�����䷴����
    DirectX::XMFLOAT3 FresneRf0 = { 0.0f,0.0f,0.0f };//������ЧӦ��������Rf��0�㣩
    float Roughness = 0.0f;//���ʴֲڶ�
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
//�ͷŶ�ָ̬��
#ifndef ReleaseCom
#define ReleaseCom(x) { if(x){ x->Release(); x = 0; } }
#endif
