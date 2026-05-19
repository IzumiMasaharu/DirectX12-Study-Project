#include "DescriptorHeapManager.h"

void DescriptorHeapManager::initialize(
	ID3D12Device* inDevice,
	D3D12_DESCRIPTOR_HEAP_TYPE type,
	UINT descriptorCount,
	bool shaderVisible)
{
	device = inDevice;
	capacity = descriptorCount;
	descriptorSize = device->GetDescriptorHandleIncrementSize(type);

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = type;
	desc.NumDescriptors = descriptorCount;
	desc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	desc.NodeMask = 0;

	ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapManager::cpuHandle(UINT index) const
{
	assert(index < capacity);
	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(descriptorHeap->GetCPUDescriptorHandleForHeapStart());
	handle.Offset(static_cast<INT>(index), descriptorSize);
	return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeapManager::gpuHandle(UINT index) const
{
	assert(index < capacity);
	CD3DX12_GPU_DESCRIPTOR_HANDLE handle(descriptorHeap->GetGPUDescriptorHandleForHeapStart());
	handle.Offset(static_cast<INT>(index), descriptorSize);
	return handle;
}

void SrvDescriptorHeap::initialize(ID3D12Device* device, UINT textureCount)
{
	heap.initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, textureCount, true);
}

void SrvDescriptorHeap::createTextureSrv(ID3D12Device* device, const Texture& texture)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = buildTextureSrvDesc(texture.resource.Get());
	device->CreateShaderResourceView(texture.resource.Get(), &srvDesc, heap.cpuHandle(texture.srvHeapIndex));
}

D3D12_SHADER_RESOURCE_VIEW_DESC SrvDescriptorHeap::buildTextureSrvDesc(ID3D12Resource* resource)
{
	D3D12_RESOURCE_DESC desc = resource->GetDesc();
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = desc.Format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	switch (desc.Dimension)
	{
	case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
		srvDesc.ViewDimension = (desc.DepthOrArraySize > 1) ? D3D12_SRV_DIMENSION_TEXTURE1DARRAY : D3D12_SRV_DIMENSION_TEXTURE1D;
		if (srvDesc.ViewDimension == D3D12_SRV_DIMENSION_TEXTURE1D)
		{
			srvDesc.Texture1D.MostDetailedMip = 0;
			srvDesc.Texture1D.MipLevels = desc.MipLevels;
			srvDesc.Texture1D.ResourceMinLODClamp = 0.0f;
		}
		else
		{
			srvDesc.Texture1DArray.MostDetailedMip = 0;
			srvDesc.Texture1DArray.MipLevels = desc.MipLevels;
			srvDesc.Texture1DArray.FirstArraySlice = 0;
			srvDesc.Texture1DArray.ArraySize = desc.DepthOrArraySize;
			srvDesc.Texture1DArray.ResourceMinLODClamp = 0.0f;
		}
		break;

	case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
		if (desc.DepthOrArraySize == 6)
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
			srvDesc.TextureCube.MostDetailedMip = 0;
			srvDesc.TextureCube.MipLevels = desc.MipLevels;
			srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
		}
		else if (desc.DepthOrArraySize > 6 && desc.DepthOrArraySize % 6 == 0)
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
			srvDesc.TextureCubeArray.MostDetailedMip = 0;
			srvDesc.TextureCubeArray.MipLevels = desc.MipLevels;
			srvDesc.TextureCubeArray.First2DArrayFace = 0;
			srvDesc.TextureCubeArray.NumCubes = desc.DepthOrArraySize / 6;
			srvDesc.TextureCubeArray.ResourceMinLODClamp = 0.0f;
		}
		else if (desc.SampleDesc.Count > 1)
		{
			srvDesc.ViewDimension = (desc.DepthOrArraySize > 1) ? D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY : D3D12_SRV_DIMENSION_TEXTURE2DMS;
			if (srvDesc.ViewDimension == D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY)
			{
				srvDesc.Texture2DMSArray.FirstArraySlice = 0;
				srvDesc.Texture2DMSArray.ArraySize = desc.DepthOrArraySize;
			}
		}
		else if (desc.DepthOrArraySize > 1)
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			srvDesc.Texture2DArray.MostDetailedMip = 0;
			srvDesc.Texture2DArray.MipLevels = desc.MipLevels;
			srvDesc.Texture2DArray.FirstArraySlice = 0;
			srvDesc.Texture2DArray.ArraySize = desc.DepthOrArraySize;
			srvDesc.Texture2DArray.PlaneSlice = 0;
			srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
		}
		else
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.MipLevels = desc.MipLevels;
			srvDesc.Texture2D.PlaneSlice = 0;
			srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		}
		break;

	case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
		srvDesc.Texture3D.MostDetailedMip = 0;
		srvDesc.Texture3D.MipLevels = desc.MipLevels;
		srvDesc.Texture3D.ResourceMinLODClamp = 0.0f;
		break;

	default:
		assert(false && "Unsupported texture dimension.");
		break;
	}

	return srvDesc;
}
