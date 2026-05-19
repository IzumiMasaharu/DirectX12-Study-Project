#pragma once

#include "D3D12Utility.h"
#include "Texture.h"

class DescriptorHeapManager
{
public:
	DescriptorHeapManager() = default;
	~DescriptorHeapManager() = default;

	void initialize(
		ID3D12Device* device,
		D3D12_DESCRIPTOR_HEAP_TYPE type,
		UINT descriptorCount,
		bool shaderVisible);

	ID3D12DescriptorHeap* getHeap() const { return descriptorHeap.Get(); }
	UINT getDescriptorSize() const { return descriptorSize; }

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle(UINT index) const;
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle(UINT index) const;

private:
	ID3D12Device* device = nullptr;
	UINT descriptorSize = 0;
	UINT capacity = 0;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;
};

class SrvDescriptorHeap
{
public:
	SrvDescriptorHeap() = default;
	~SrvDescriptorHeap() = default;

	void initialize(ID3D12Device* device, UINT textureCount);
	void createTextureSrv(ID3D12Device* device, const Texture& texture);

	ID3D12DescriptorHeap* getHeap() const { return heap.getHeap(); }
	D3D12_GPU_DESCRIPTOR_HANDLE gpuStart() const { return heap.gpuHandle(0); }
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle(UINT index) const { return heap.gpuHandle(index); }
	UINT getDescriptorSize() const { return heap.getDescriptorSize(); }

private:
	static D3D12_SHADER_RESOURCE_VIEW_DESC buildTextureSrvDesc(ID3D12Resource* resource);

private:
	DescriptorHeapManager heap;
};
