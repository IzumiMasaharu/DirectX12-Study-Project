#pragma once

#include "D3D12Utility.h"
#include "Geometry.h"
#include "RenderLayer.h"
#include "ShaderResource.h"

#include <string>
#include <vector>

#ifndef MAYOHOSHI_NUM_FRAME_RESOURCES_DEFINED
#define MAYOHOSHI_NUM_FRAME_RESOURCES_DEFINED
const UINT gNumFrameResources = 3; // 程序所用帧资源总数
#endif

struct RenderItem
{
public:
	RenderItem() = default;

public:
	std::string name;
	std::vector<InstanceData> instances;

	UINT numDirtyFrames = gNumFrameResources;
	UINT instanceBufferOffset = 0;

	MeshGeometry* Geo = nullptr;

	RenderLayer renderLayer = RenderLayers::Opaque;
	D3D12_PRIMITIVE_TOPOLOGY primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	UINT indexCount = 0;
	uint32_t indexStartLocation = 0;
	uint32_t vertexBaseLocation = 0;
};
