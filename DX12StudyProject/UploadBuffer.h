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

		ThrowIfFailed(mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData)))//��ɶ�GPU�����Դ�ڴ�λ�õ�ӳ��
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
	//�����ݿ�����CPU�У�����ͨ����Դӳ����ʱ����CPU�е���Դ���ݣ��Ӷ�����GPU��Ⱦ�����е�����
	void CopyData(int ElementIndex, const T& data)
	{
		memcpy(&mMappedData[ElementIndex * mElementByteSize], &data, sizeof(T));
	}
	//����ָ�򻺳�����Դ��ָ��
	ID3D12Resource* Resource()const
	{
		return mUploadBuffer.Get();
	}
private:
	Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer;//ָ���������ϴ���������ָ��
	BYTE* mMappedData = nullptr;//ָ��ӳ�����ݿ��ָ��
	uint32_t mElementByteSize = 0;//�ϴ��������洢�������͵Ĵ�С
	bool mIsConstantBuffer = false;
};