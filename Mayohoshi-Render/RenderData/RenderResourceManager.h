#pragma once

#include "DDSTextureLoader.h"
#include "D3D12Utility.h"
#include "FrameResource.h"
#include "Geometry.h"
#include "GeometryGenerator.h"
#include "Material.h"
#include "RenderLayer.h"
#include "Texture.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

enum class GeometryType
{
    Cylinder,
    Sphere,
    Grid,
    ObjModel,
};

struct SubmeshGeometryParams
{
    SubmeshGeometryParams() = default;

    GeometryType type = GeometryType::Grid;
    std::string name;

    union
    {
        struct
        {
            float bottomRadius;
            float topRadius;
            float height;
            uint32_t sliceCount;
            uint32_t stackCount;
        } cylinder;

        struct
        {
            float radius;
            uint32_t sliceCount;
            uint32_t stackCount;
        } sphere;

        struct
        {
            float width;
            float depth;
            uint32_t m;
            uint32_t n;
        } grid;
    };

    std::wstring objFilePath;

    static SubmeshGeometryParams Cylinder(
        const std::string& name,
        float bottomRadius,
        float topRadius,
        float height,
        uint32_t sliceCount,
        uint32_t stackCount);

    static SubmeshGeometryParams Sphere(
        const std::string& name,
        float radius,
        uint32_t sliceCount,
        uint32_t stackCount);

    static SubmeshGeometryParams Grid(
        const std::string& name,
        float width,
        float depth,
        uint32_t m,
        uint32_t n);

    static SubmeshGeometryParams ImportedOBJ(
        const std::string& name,
        const std::wstring& filePath);
};

struct MaterialDesc
{
    std::string name;

    DirectX::XMFLOAT4 albedo = { 1.0f,1.0f,1.0f,1.0f };
    float metallic = 0.0f;
    float roughness = 0.0f;
    float ior = 1.0f;
    DirectX::XMFLOAT3 emissive = { 0.0f,0.0f,0.0f };
    DirectX::XMFLOAT4X4 materialTransform = MathUtilities::Identity4x4();
    RenderLayer renderLayer = RenderLayers::Opaque;
};

struct TextureTableDesc
{
    std::string name;
    std::string diffuseTexture;
    std::string normalTexture;
    std::string depthTexture;
};

class RenderResource
{
    friend class GeometryManager;
    friend class MaterialManager;
    friend class TextureManager;
    friend class TextureTableManager;

public:
    RenderResource() = default;
    ~RenderResource() = default;

    MeshGeometry* getMeshGeometry(const std::string& name) const;
    Material* getMaterial(const std::string& name) const;
    Texture* getTexture(const std::string& name) const;
    const TextureTable* getTextureTable(const std::string& name) const;
    UINT getTextureTableIndex(const std::string& name) const;

    size_t getMeshGeometryCount() const { return meshes.size(); }
    size_t getMaterialCount() const { return materials.size(); }
    size_t getTextureCount() const { return textures.size(); }
    size_t getTextureTableCount() const { return textureTables.size(); }

    void forEachTexture(const std::function<void(const Texture&)>& callback) const;
    void forEachMaterial(const std::function<void(Material*)>& callback) const;
    void forEachTextureTable(const std::function<void(UINT, const TextureTable&)>& callback) const;

private:
    bool addGeometry(std::unique_ptr<MeshGeometry> mesh);
    bool addMaterial(std::unique_ptr<Material> material);
    bool addTexture(std::unique_ptr<Texture> texture);
    bool addTextureTable(const std::string& name, const TextureTable& table);

    UINT allocateMaterialIndex() { return nextMaterialIndex++; }
    UINT allocateSrvIndex() { return nextSrvIndex++; }
    UINT allocateTextureTableIndex() { return nextTextureTableIndex++; }

private:
    std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> meshes;
    std::unordered_map<std::string, std::unique_ptr<Material>> materials;
    std::unordered_map<std::string, std::unique_ptr<Texture>> textures;
    std::unordered_map<std::string, UINT> textureTableNameToIndex;
    std::vector<TextureTable> textureTables;

    UINT nextMaterialIndex = 0;
    UINT nextSrvIndex = 0;
    UINT nextTextureTableIndex = 0;
};

class RenderResourceManager
{
public:
    RenderResourceManager() = default;
    explicit RenderResourceManager(ID3D12Device* device) : device(device) { }
    ~RenderResourceManager() = default;

    void initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
    void setCmdList(ID3D12GraphicsCommandList* cmdList) { commandList = cmdList; }

    bool createMeshGeometry(
        const std::string& name,
        bool isUse32BitIndices,
        const std::vector<SubmeshGeometryParams>& params);

    bool createMaterial(const MaterialDesc& matDesc);
    bool createTexture(const std::string& name, const std::wstring& filePath);
    bool createTextureTable(const TextureTableDesc& desc);

    MeshGeometry* getMeshGeometry(const std::string& name) const { return renderResource.getMeshGeometry(name); }
    Material* getMaterial(const std::string& name) const { return renderResource.getMaterial(name); }
    Texture* getTexture(const std::string& name) const { return renderResource.getTexture(name); }
    const TextureTable* getTextureTable(const std::string& name) const { return renderResource.getTextureTable(name); }
    UINT getTextureTableIndex(const std::string& name) const { return renderResource.getTextureTableIndex(name); }

    size_t getMaterialCount() const { return renderResource.getMaterialCount(); }
    size_t getTextureCount() const { return renderResource.getTextureCount(); }
    size_t getTextureTableCount() const { return renderResource.getTextureTableCount(); }

    void forEachTexture(const std::function<void(const Texture&)>& callback) const { renderResource.forEachTexture(callback); }
    void forEachMaterial(const std::function<void(Material*)>& callback) const { renderResource.forEachMaterial(callback); }
    void forEachTextureTable(const std::function<void(UINT, const TextureTable&)>& callback) const { renderResource.forEachTextureTable(callback); }

private:
    RenderResource renderResource;
    ID3D12Device* device = nullptr;
    ID3D12GraphicsCommandList* commandList = nullptr;
};

class GeometryManager
{
    friend class RenderResourceManager;

protected:
    static bool createMeshGeometry(
        RenderResource& resource,
        ID3D12Device* device,
        ID3D12GraphicsCommandList* commandList,
        const std::string& meshName,
        bool isUse32BitIndices,
        const std::vector<SubmeshGeometryParams>& submeshParams);

    template<typename IndexType>
    static bool createMeshGeometryImpl(
        RenderResource& resource,
        ID3D12Device* device,
        ID3D12GraphicsCommandList* commandList,
        const std::string& meshName,
        const std::vector<SubmeshGeometryParams>& submeshParams);

    static GeometryGenerator::MeshData buildMeshData(const SubmeshGeometryParams& params);
};

class MaterialManager
{
    friend class RenderResourceManager;

protected:
    static bool createMaterial(RenderResource& resource, const MaterialDesc& matDesc);
};

class TextureManager
{
    friend class RenderResourceManager;

protected:
    static bool loadTexture(
        RenderResource& resource,
        ID3D12Device* device,
        ID3D12GraphicsCommandList* commandList,
        const std::string& name,
        const std::wstring& filePath);
};

class TextureTableManager
{
    friend class RenderResourceManager;

protected:
    static bool createTextureTable(RenderResource& resource, const TextureTableDesc& desc);
};
