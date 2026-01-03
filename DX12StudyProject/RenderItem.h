#pragma once
#include "DXBase.h"

enum class TextureType : uint32_t
{
	TEX_NONE = 0u,
	TEX_DIFFUSE = 1u << 0,
	TEX_NORMAL = 1u << 1,
	TEX_DEPTH = 1u << 2,
};

inline TextureType operator|(TextureType a, TextureType b)
{
	return static_cast<TextureType>(
		static_cast<uint32_t>(a) |
		static_cast<uint32_t>(b)
		);
}

inline TextureType operator&(TextureType a, TextureType b)
{
	return static_cast<TextureType>(
		static_cast<uint32_t>(a) &
		static_cast<uint32_t>(b)
		);
}

inline bool HasFlag(TextureType value, TextureType flag)
{
	return (static_cast<uint32_t>(value) &
		static_cast<uint32_t>(flag)) != 0;
}

const UINT gNumFrameResources = 3; // 程序所用帧资源总数

struct RenderItem
{
public:
	RenderItem() = default;
public:
	DirectX::XMFLOAT4X4 worldTransform = MathHelper::Identity4x4();		// 渲染项的世界变换矩阵
	DirectX::XMFLOAT4X4 textureTransform = MathHelper::Identity4x4();	// UV偏移矩阵
	UINT materialIndex; // 该渲染项所使用的材质

	TextureType textureFlags; // 纹理标记
	std::array<UINT, MAX_BINDING_TEXTURE> diffuseTextureIndex;
	std::array<UINT, MAX_BINDING_TEXTURE> normalTextureIndex;
	std::array<UINT, MAX_BINDING_TEXTURE> depthTextureIndex;

	UINT numDirtyFrames = gNumFrameResources; // 记录有几个帧资源中的渲染项数据待更新
	UINT objectConstBufferIndex = -1; // 该渲染项的常量缓冲区在所有渲染项的缓冲区中的索引
	
	MeshGeometry* Geo = nullptr; // 该渲染项所使用的网格体集
	D3D12_PRIMITIVE_TOPOLOGY primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST; // 指定渲染项的图元拓扑格式
	// 以下三个变量均与Geo所绑定的网格体相关
	UINT indexCount = 0;
	uint32_t indexStartLocation = 0;
	uint32_t vertexBaseLocation = 0;
};