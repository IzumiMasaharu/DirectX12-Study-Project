#pragma once
#include "DXBase.h"

const UINT gNumFrameResources = 3;//程序所用帧资源总数

struct RenderItem
{
public:
	RenderItem() = default;
public:
	DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();//渲染项的世界变换矩阵
	DirectX::XMFLOAT4X4 TexTrans = MathHelper::Identity4x4();

	UINT numDirtyFrames = gNumFrameResources;//记录有几个帧资源中的渲染项数据待更新
	UINT ObjectConstBufferIndex = -1;//该渲染项的常量缓冲区在所有渲染项的缓冲区中的索引

	Material* Mat = nullptr;//该渲染项所使用的材质
	MeshGeometry* Geo = nullptr;//该渲染项所使用的网格体集
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;//指定渲染项的图元拓扑格式
	//以下三个变量均与Geo所绑定的网格体相关
	UINT indexCount = 0;
	uint32_t indexStartLocation = 0;
	uint32_t vertexBaseLocation = 0;
};