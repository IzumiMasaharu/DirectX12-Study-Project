#include "RenderSceneManager.h"
#include <algorithm>
#include <DirectXMath.h>

RenderItem* RenderSceneManager::createRenderItem(
    const RenderItemDesc& desc,
    const RenderResourceManager& resourceManager)
{
    if (desc.name.empty() || nameToIndex.find(desc.name) != nameToIndex.end())
        return nullptr;

    MeshGeometry* meshGeo = resourceManager.getMeshGeometry(desc.geoName);
    if (!meshGeo)
        return nullptr;

    auto submeshIt = meshGeo->submeshList.find(desc.submeshName);
    if (submeshIt == meshGeo->submeshList.end())
        return nullptr;

    auto item = std::make_unique<RenderItem>();
    item->Geo = meshGeo;
    item->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    item->indexCount = submeshIt->second.indexCount;
    item->indexStartLocation = submeshIt->second.indexStartLocation;
    item->vertexBaseLocation = submeshIt->second.vertexBaseLocation;
    item->numDirtyFrames = gNumFrameResources;
	item->renderLayer = desc.renderLayer;

    RenderItem* out = item.get();

    const size_t newIndex = renderItems.size();
    renderItems.emplace_back(std::move(item));
    indexToName.emplace_back(desc.name);
    nameToIndex.emplace(desc.name, newIndex);

    return out;
}

bool RenderSceneManager::removeRenderItem(const std::string& name)
{
    auto it = nameToIndex.find(name);
    if (it == nameToIndex.end())
        return false;

    const size_t removeIndex = it->second;
    const size_t lastIndex = renderItems.size() - 1;

    if (removeIndex != lastIndex)
    {
        renderItems[removeIndex] = std::move(renderItems[lastIndex]);
        indexToName[removeIndex] = std::move(indexToName[lastIndex]);
        nameToIndex[indexToName[removeIndex]] = removeIndex;
    }

    renderItems.pop_back();
    indexToName.pop_back();
    nameToIndex.erase(it);

    return true;
}

RenderItem* RenderSceneManager::getRenderItem(const std::string& name)
{
    auto it = nameToIndex.find(name);
    if (it == nameToIndex.end())
        return nullptr;
    return renderItems[it->second].get();
}

bool RenderSceneManager::addInstance(const std::string& renderItemName, const InstanceDesc& instanceDesc)
{
    RenderItem* item = getRenderItem(renderItemName);
    if (!item)
        return false;

    InstanceData data;
    data.worldTransform = instanceDesc.worldTransform;
	Material* mat = 

    item->instances.emplace_back(data);
    item->numDirtyFrames = gNumFrameResources;
    return true;
}

bool RenderSceneManager::clearInstances(const std::string& renderItemName)
{
    RenderItem* item = getRenderItem(renderItemName);
    if (!item)
        return false;

    item->instances.clear();
    item->numDirtyFrames = gNumFrameResources;
    return true;
}

void RenderSceneManager::clearAllInstances()
{
    for (const auto& item : renderItems)
    {
        item->instances.clear();
        item->numDirtyFrames = gNumFrameResources;
    }
}

void RenderSceneManager::buildRenderQueue(const DirectX::XMFLOAT3& cameraPos, bool sortTransparentInstances)
{
    sortedEntries.clear();
    sortedRenderItems.clear();
    layerBuckets.clear();

    sortedEntries.reserve(renderItems.size());
    sortedRenderItems.reserve(renderItems.size());

    for (size_t i = 0; i < renderItems.size(); ++i)
    {
        RenderItem* item = renderItems[i].get();
        if (!item || !item->material)
            continue;

        const RenderLayer layer = resolveLayer(*item);

        if (sortTransparentInstances && isTransparentLikeLayer(layer) && item->instances.size() > 1)
        {
            std::stable_sort(
                item->instances.begin(),
                item->instances.end(),
                [&cameraPos](const InstanceData& a, const InstanceData& b)
                {
                    return calcInstanceDistanceSq(a, cameraPos) > calcInstanceDistanceSq(b, cameraPos);
                });
            item->numDirtyFrames = gNumFrameResources;
        }

        SortEntry entry;
        entry.item = item;
        entry.layer = layer;
        entry.distanceSq = calcItemDistanceSq(*item, cameraPos);
        entry.createOrder = i;
        sortedEntries.emplace_back(entry);
    }

    std::stable_sort(
        sortedEntries.begin(),
        sortedEntries.end(),
        [this](const SortEntry& a, const SortEntry& b)
        {
            if (a.layer != b.layer)
                return a.layer < b.layer;

            const bool transparentLike = isTransparentLikeLayer(a.layer);
            if (a.distanceSq != b.distanceSq)
            {
                if (transparentLike)
                    return a.distanceSq > b.distanceSq; // back-to-front
                return a.distanceSq < b.distanceSq;     // front-to-back
            }

            return a.createOrder < b.createOrder;
        });

    for (const auto& entry : sortedEntries)
    {
        sortedRenderItems.emplace_back(entry.item);
        layerBuckets[entry.layer].emplace_back(entry.item);
    }
}

const std::vector<RenderItem*>& RenderSceneManager::getRenderItemsByLayer(RenderLayer layer) const
{
    auto it = layerBuckets.find(layer);
    if (it == layerBuckets.end())
        return 
    return it->second;
}

void RenderSceneManager::forEachRenderItem(const std::function<void(RenderItem*, RenderLayer)>& callback) const
{
    for (const auto& entry : sortedEntries)
    {
        callback(entry.item, entry.layer);
    }
}

void RenderSceneManager::clear()
{
    renderItems.clear();
    indexToName.clear();
    nameToIndex.clear();

    sortedEntries.clear();
    sortedRenderItems.clear();
    layerBuckets.clear();

    createSequence = 0;
}

size_t RenderSceneManager::getTotalInstanceCount() const
{
    size_t total = 0;
    for (const auto& item : renderItems)
        total += item->instances.size();
    return total;
}

DirectX::XMFLOAT3 RenderSceneManager::extractTranslation(const DirectX::XMFLOAT4X4& m)
{
    return DirectX::XMFLOAT3(m._41, m._42, m._43);
}

float RenderSceneManager::calcDistanceSq(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b)
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    const float dz = a.z - b.z;
    return dx * dx + dy * dy + dz * dz;
}

float RenderSceneManager::calcInstanceDistanceSq(const InstanceData& instance, const DirectX::XMFLOAT3& cameraPos)
{
    const DirectX::XMFLOAT3 p = extractTranslation(instance.worldTransform);
    return calcDistanceSq(p, cameraPos);
}

float RenderSceneManager::calcItemDistanceSq(const RenderItem& item, const DirectX::XMFLOAT3& cameraPos)
{
    if (item.instances.empty())
        return 0.0f;

    DirectX::XMFLOAT3 center = { 0.0f, 0.0f, 0.0f };
    for (const auto& ins : item.instances)
    {
        const DirectX::XMFLOAT3 p = extractTranslation(ins.worldTransform);
        center.x += p.x;
        center.y += p.y;
        center.z += p.z;
    }

    const float inv = 1.0f / static_cast<float>(item.instances.size());
    center.x *= inv;
    center.y *= inv;
    center.z *= inv;

    return calcDistanceSq(center, cameraPos);
}

RenderLayer RenderSceneManager::resolveLayer(const RenderItem& item) const
{
	return item.renderLayer;
}

bool RenderSceneManager::isTransparentLikeLayer(RenderLayer layer) const
{
    return layer >= RenderLayers::Transparent;
}