#pragma once

#include "RenderResourceManager.h"
#include "RenderItem.h"
#include <DirectXMath.h>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct RenderItemDesc
{
    std::string name;
    std::string geoName;
    std::string submeshName;
	RenderLayer renderLayer = RenderLayers::Opaque;
};

struct InstanceDesc
{
    DirectX::XMFLOAT4X4 worldTransform = MathHelper::Identity4x4();
	std::string materialName;
};

class RenderSceneManager
{
public:
    RenderSceneManager() = default;
    ~RenderSceneManager() = default;

    RenderItem* createRenderItem(const RenderItemDesc& desc, const RenderResourceManager& resourceManager);
    bool removeRenderItem(const std::string& name);

    RenderItem* getRenderItem(const std::string& name);

    bool addInstance(const std::string& renderItemName, const InstanceDesc& instanceDesc);
    bool clearInstances(const std::string& renderItemName);
    void clearAllInstances();

    // 每帧绘制前调用：构建排序结果（可选地对透明层实例排序）
    void buildRenderQueue(const DirectX::XMFLOAT3& cameraPos, bool sortTransparentInstances = true);

    const std::vector<RenderItem*>& getSortedRenderItems() const { return sortedRenderItems; }
    const std::vector<RenderItem*>& getRenderItemsByLayer(RenderLayer layer) const;

    void forEachRenderItem(const std::function<void(RenderItem*, RenderLayer)>& callback) const;

    void clear();

    size_t getRenderItemCount() const { return renderItems.size(); }
    size_t getTotalInstanceCount() const;

private:
    struct SortEntry
    {
        RenderItem* item = nullptr;
        RenderLayer layer = RenderLayers::Opaque;
        float distanceSq = 0.0f;
        uint64_t createOrder = 0;
    };

    static DirectX::XMFLOAT3 extractTranslation(const DirectX::XMFLOAT4X4& m);
    static float calcDistanceSq(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b);
    static float calcInstanceDistanceSq(const InstanceData& instance, const DirectX::XMFLOAT3& cameraPos);
    static float calcItemDistanceSq(const RenderItem& item, const DirectX::XMFLOAT3& cameraPos);

    RenderLayer resolveLayer(const RenderItem& item) const;
    bool isTransparentLikeLayer(RenderLayer layer) const;

private:
    std::vector<std::unique_ptr<RenderItem>> renderItems;
    std::vector<std::string> indexToName;
    std::unordered_map<std::string, size_t> nameToIndex;

    std::vector<SortEntry> sortedEntries;
    std::vector<RenderItem*> sortedRenderItems;
    std::unordered_map<RenderLayer, std::vector<RenderItem*>> layerBuckets;

    uint64_t createSequence = 0;
};