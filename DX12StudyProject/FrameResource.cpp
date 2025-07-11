#include "FrameResource.h"

FrameResource::FrameResource(ID3D12Device* device, UINT PassCount, UINT ObjectCount,UINT MaterialCount)
{
	ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(commandAllocator.GetAddressOf())))

	passConstBuffer = std::make_unique<UploadBuffer<RenderingPassConstants>>(device, PassCount, true);
	objectConstBuffer = std::make_unique<UploadBuffer<ObjectConstants>>(device, ObjectCount, true);
	materialConstBuffer = std::make_unique<UploadBuffer<MaterialConstants>>(device, MaterialCount, true);
}