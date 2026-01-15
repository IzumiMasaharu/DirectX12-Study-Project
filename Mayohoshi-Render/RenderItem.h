#pragma once
const UINT gNumFrameResources = 3;	// 程序所用帧资源总数

struct RenderItem
{
public:
	RenderItem() = default;
public:
	std::vector<InstanceData> instances;		// 存储该渲染项的所有实例数据（每个实例对应一个物体常量缓冲区）

	UINT numDirtyFrames = gNumFrameResources;	// 记录有几个帧资源中的渲染项数据待更新
	UINT instanceBufferOffset = 0;				// 实例在缓冲区中的起始偏移
	
	MeshGeometry* Geo = nullptr;				// 该渲染项所使用的网格体集
	D3D12_PRIMITIVE_TOPOLOGY primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;	// 指定渲染项的图元拓扑格式

	// 以下三个变量均与Geo所绑定的网格体相关
	UINT indexCount = 0;
	uint32_t indexStartLocation = 0;
	uint32_t vertexBaseLocation = 0;
};