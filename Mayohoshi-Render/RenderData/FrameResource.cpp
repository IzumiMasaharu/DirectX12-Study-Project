#include "FrameResource.h"

FrameResource::FrameResource(ID3D12Device* device, UINT passCount, UINT instanceCount, UINT materialCount, UINT textureTableCount)
{
	ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(commandAllocator.GetAddressOf())));

	passConstBuffer = std::make_unique<UploadBuffer<RenderingPassConstants>>(device, passCount, true);

	instanceStructuredBuffer = std::make_unique<UploadBuffer<InstanceData>>(device, instanceCount, false);
	materialStructuredBuffer = std::make_unique<UploadBuffer<MaterialData>>(device, materialCount, false);
	textureTableBuffer = std::make_unique<UploadBuffer<TextureTable>>(device, textureTableCount, false);
}
