#include "FrameResource.h"

FrameResource::FrameResource(ID3D12Device* device, UINT PassCount, UINT ObjectCount,UINT MaterialCount)
{
	ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(CommandAllocator.GetAddressOf())))

	PassConstBuffer = std::make_unique<UploadBuffer<RenderingPassConstants>>(device, PassCount, true);
	ObjectConstBuffer = std::make_unique<UploadBuffer<ObjectConstants>>(device, ObjectCount, true);
	MaterialConstBuffer = std::make_unique<UploadBuffer<MaterialConstants>>(device, MaterialCount, true);
}