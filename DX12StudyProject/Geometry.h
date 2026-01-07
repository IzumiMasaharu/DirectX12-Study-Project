#pragma once
#include <d3d12.h>
#include <DirectXCollision.h>
#include <string>
#include <unordered_map>
#include <Windows.h>
#include <wrl.h>

// 存储单个网格体的结构体
struct SubmeshGeometry
{
    std::string name;

    UINT indexCount = 0; // 索引数目
    UINT indexStartLocation = 0; // 索引开始位置
    UINT vertexBaseLocation = 0; // 基准定点位置
    DirectX::BoundingBox bounds; // 设置边界框
};

// 存储全部网格体的结构体
struct MeshGeometry
{
public:
    // 为GPU中的顶点缓冲区资源创建顶点缓冲区视图，用于绑定到Pipeline IA阶段
    D3D12_VERTEX_BUFFER_VIEW VertexBufferView()const
    {
        D3D12_VERTEX_BUFFER_VIEW vbv;
        vbv.BufferLocation = vertexBufferGPU->GetGPUVirtualAddress();
        vbv.StrideInBytes = vertexByteStride;
        vbv.SizeInBytes = vertexBufferByteSize;

        return vbv;
    }
    // 为GPU中的索引缓冲区资源创建索引缓冲区视图，用于绑定到Pipeline IA阶段
    D3D12_INDEX_BUFFER_VIEW IndexBufferView()const
    {
        D3D12_INDEX_BUFFER_VIEW ibv;
        ibv.BufferLocation = indexBufferGPU->GetGPUVirtualAddress();
        ibv.Format = indexFormat;
        ibv.SizeInBytes = indexBufferByteSize;

        return ibv;
    }
    // 在向GPU上传完资源之后释放顶点上传缓冲区和索引上传缓冲区
    void DisposeUploaders()
    {
        vertexBufferUploader = nullptr;
        indexBufferUploader = nullptr;
    }
public:
    std::string name;

    Microsoft::WRL::ComPtr<ID3DBlob> vertexBufferCPU = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> indexBufferCPU = nullptr;
    //                       ↓
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBufferUploader = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexBufferUploader = nullptr;
    //                       ↓
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBufferGPU = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexBufferGPU = nullptr;

    UINT vertexByteStride = 0; // 顶点结构体大小
    UINT vertexBufferByteSize = 0; // 顶点缓冲区大小
    UINT indexBufferByteSize = 0; // 索引缓冲区大小
    DXGI_FORMAT indexFormat = DXGI_FORMAT_R16_UINT; // 资源格式

    std::unordered_map<std::string, SubmeshGeometry> submeshList; // 将全部网格体拆分为若干单个网格体，存储到无序图中
};