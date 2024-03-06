#pragma once
#include "DXBase.h"

template<typename T>
class UploadBuffer
{
public:
	UploadBuffer(ID3D12Device* mDevice, UINT ElemenCount, bool is_ConstBuffer) : mIsConstantBuffer(is_ConstBuffer)
	{
		mElementByteSize = sizeof(T);

		if (mIsConstantBuffer)
			mElementByteSize = DXBase::ConstUploadBufferByteSize256Alignment(mElementByteSize);

		ThrowIfFailed(mDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(mElementByteSize * ElemenCount),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mUploadBuffer)))

		ThrowIfFailed(mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData)))//完成对GPU相关资源内存位置的映射
	}
	UploadBuffer(const UploadBuffer& ub) = delete;
	~UploadBuffer()
	{
		if (mUploadBuffer != nullptr)
			mUploadBuffer->Unmap(0, nullptr);
		mMappedData = nullptr;
	}
public:
	UploadBuffer& operator=(const UploadBuffer& ub) = delete;
public:
	//将数据拷贝到CPU中，即可通过资源映射随时更改CPU中的资源数据，从而更改GPU渲染管线中的数据
	void CopyData(int ElementIndex, const T& data)
	{
		memcpy(&mMappedData[ElementIndex * mElementByteSize], &data, sizeof(T));
	}
	//返回指向缓冲区资源的指针
	ID3D12Resource* Resource()const
	{
		return mUploadBuffer.Get();
	}
private:
	Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer;//指向所创建上传缓冲区的指针
	BYTE* mMappedData = nullptr;//指向映射数据块的指针
	uint32_t mElementByteSize = 0;//上传缓冲区存储数据类型的大小
	bool mIsConstantBuffer = false;
};