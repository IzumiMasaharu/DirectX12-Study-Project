#pragma once

#include "ShaderResource.h"
#include "UploadBuffer.h"

// 帧资源
struct FrameResource
{
public:
    FrameResource(ID3D12Device* device, UINT passCount, UINT instanceCount, UINT materialCount, UINT textureTableCount);
    FrameResource(const FrameResource& rhs) = delete;
    ~FrameResource() = default;

    FrameResource& operator=(const FrameResource& rhs) = delete;
public:
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator; // 每个帧资源的命令分配器
    std::unique_ptr<UploadBuffer<RenderingPassConstants>> passConstBuffer = nullptr; // 每个帧资源的渲染过程常量缓冲区
    
    std::unique_ptr<UploadBuffer<TextureTable>> textureTableBuffer = nullptr;
    std::unique_ptr<UploadBuffer<MaterialData>> materialStructuredBuffer = nullptr; // 结构化材质常量缓冲区
    std::unique_ptr<UploadBuffer<InstanceData>> instanceStructuredBuffer = nullptr; // 每个帧资源的物体常量缓冲区
    
    UINT64 fence = 0;
};

