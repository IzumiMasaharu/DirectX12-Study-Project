#pragma once
#include "DXBase.h"

template<typename T>
class UploadBuffer
{
public:
    UploadBuffer(ID3D12Device* mDevice, UINT ElemenCount, bool is_ConstBuffer) : mIsConstantBuffer(is_ConstBuffer)
    {
        mElementByteSize = sizeof(T);

        // 如果是常量缓冲区，则调整大小以满足 256 字节对齐要求
        if (mIsConstantBuffer)
            mElementByteSize = DXBase::ConstUploadBufferByteSize256Alignment(mElementByteSize);

        // 创建上传缓冲区资源（位于CPU）
        ThrowIfFailed(mDevice->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(mElementByteSize * ElemenCount),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&mUploadBuffer)));

        // 将上传缓冲区资源映射到CPU地址空间（将mMapppedData与mUploadBuffer相关联）
        ThrowIfFailed(mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData)));
    }

    UploadBuffer(const UploadBuffer& ub) = delete;

    ~UploadBuffer()
    {
        // 取消映射缓冲区
        if (mUploadBuffer != nullptr)
            mUploadBuffer->Unmap(0, nullptr);
        mMappedData = nullptr;
    }

    UploadBuffer& operator=(const UploadBuffer& ub) = delete;

    // 将数据拷贝到上传缓冲区
    void CopyData(int ElementIndex, const T& data)
    {
        memcpy(&mMappedData[ElementIndex * mElementByteSize], &data, sizeof(T));
    }

    // 返回指向缓冲区资源的指针
    ID3D12Resource* Resource() const
    {
        return mUploadBuffer.Get();
    }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer; // 指向所创建上传缓冲区的指针
    BYTE* mMappedData = nullptr; // 指向上传缓冲区映射到的数据块的指针
    uint32_t mElementByteSize = 0; // 上传缓冲区存储数据类型的大小
    bool mIsConstantBuffer = false; // 是否为常量缓冲区
};