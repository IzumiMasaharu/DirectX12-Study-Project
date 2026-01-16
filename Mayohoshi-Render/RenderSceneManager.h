#pragma once

#include "DxUtil.h"
#include "RenderResourceManager.h"
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

class RenderSceneManager
{
public:
	RenderSceneManager() = default;
	~RenderSceneManager() = default;
private:
	RenderScene renderScene;
};

class RenderScene
{
	friend class RenderItemManager;
	friend class InstanceManager;
public:
	RenderScene() = default;
	~RenderScene() = default;
protected:
	bool addRenderItem(RenderItemType type, std::unique_ptr<RenderItem> renderItem)
	{
		switch (type)
		{
		case RenderItemType::OpaqueRenderItem:
			renderItemManager.addOpaqueRenderItem(std::move(renderItem));
			break;
		case RenderItemType::SkycubeRenderItem:
			skycubeRenderItem.push_back(std::move(renderItem));
			allRenderItems.push_back(skycubeRenderItem.back().get());
			break;
		case RenderItemType::TransparentRenderItem:
			transparentRenderItems.push_back(std::move(renderItem));
			allRenderItems.push_back(transparentRenderItems.back().get());
			break;
		default:
			return false;
		}

		return true;
	}
	bool addInstanceData(const InstanceData& instanceData)
	{
		instancePool.instanceDatas.push_back(instanceData);
		return true;
	}

private:
	RenderItemLibrary renderItemManager;
	InstancePool instancePool;
};

class RenderItemLibrary
{
	friend class RenderScene;

public:
	RenderItemLibrary() = default;
	~RenderItemLibrary() = default;

protected:
	bool addOpaqueRenderItem(std::unique_ptr<RenderItem> renderItem)
	{
		opaqueRenderItems.push_back(std::move(renderItem));
		allRenderItems.push_back(opaqueRenderItems.back().get());

		return true;
	}
private:
	std::vector<RenderItem*> allRenderItems;							// 储存有所有渲染项
};

class InstancePool
{
public:
	InstancePool() = default;
	~InstancePool() = default;


public:
	std::vector<InstanceData> instanceDatas;
};

class RenderItemManager
{
	friend class RenderSceneManager;
public:
	RenderItemManager() = default;
	~RenderItemManager() = default;
protected:
	static bool buildRenderItem(RenderItemAttributes attributes, RenderScene& renderScene ,const RenderResourceManager& renderResourceManager)
	{
		auto renderItem = std::make_unique<RenderItem>();
		renderItem->Geo = renderResourceManager.getMeshGeometry(attributes.geoName);
		renderItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		renderItem->indexCount = renderItem->Geo->submeshList[attributes.submeshName].indexCount;
		renderItem->indexStartLocation = renderItem->Geo->submeshList[attributes.submeshName].indexStartLocation;
		renderItem->vertexBaseLocation = renderItem->Geo->submeshList[attributes.submeshName].vertexBaseLocation;

		return renderScene.addRenderItem(attributes.type, std::move(renderItem));
	}

private:

};

class InstanceManager
{
public:
	InstanceManager() = default;
	~InstanceManager() = default;

};