#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <memory>
#include <d3d12.h>
#include <Windows.h>
#include "Material.h"
#include "Texture.h"
#include "Geometry.h"
#include "RenderItem.h"


enum class RenderItemType
{
	OpaqueRenderItem,
	TransparentRenderItem,
	SkycubeRenderItem,
};

struct RenderItemAttributes
{
	std::string renderItemName;
	std::string geoName;
	std::string submeshName;
	D3D12_PRIMITIVE_TOPOLOGY primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	RenderItemType type;
};

class RenderResources
{
private:
	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> geos;				// 储存几何网格体的无序图
	std::unordered_map<std::string, std::unique_ptr<Material>> materials;				// 存储材质的无序图
	std::unordered_map<std::string, std::unique_ptr<Texture>> textures;					// 存储纹理的无序图
};

class RenderSourceManager
{
	RenderItemManager renderItemManager;
	GeometryManager geometryNanager;
	RenderResources renderSourceLibrary;
};
class RenderItemManager
{
public:
	RenderItemManager() = default;
	~RenderItemManager() = default;
	void addRenderItem(RenderItemAttributes attributes)
	{
		auto renderItem = std::make_unique<RenderItem>();
		renderItem->Geo = geos["Geo"].get();
		renderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		renderItem->indexCount = renderItem->Geo->submeshList["Geo_Cylinder"].indexCount;
		renderItem->indexStartLocation = renderItem->Geo->submeshList["Geo_Cylinder"].indexStartLocation;
		renderItem->vertexBaseLocation = renderItem->Geo->submeshList["Geo_Cylinder"].vertexBaseLocation;
		switch (attributes.type)
		{

		}
	}

private:
	
};
class GeometryManager
{

};
