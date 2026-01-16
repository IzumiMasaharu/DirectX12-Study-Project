#pragma once

#include "DxUtil.h"
#include "DDSTextureLoader.h"
#include "FrameResource.h"
#include <winnt.h>
#include <string>
#include <unordered_map>
#include <memory>
#include "Geometry.h"
#include "GeometryGenerator.h"
#include "Material.h"
#include "Texture.h"

class RenderResourceManager
{
public:
    explicit RenderResourceManager(ID3D12Device* device): device(device) { }
	RenderResourceManager() = delete;
    ~RenderResourceManager() = default;

    void setCmdList(ID3D12GraphicsCommandList* cmdList)
    {
		this->commandList = cmdList;
    }

    // 统一的创建几何体API
    bool createMeshGeometry(
        const std::string name,
        const bool isUse32BitIndices,
        const std::vector<SubmeshGeometryParams>& params)
    {
		return GeometryManager::createMeshGeometry(
            renderResource, device, commandList, name, isUse32BitIndices, params);
    }
    bool createMaterial(const MaterialDesc& matDesc)
    {
		return MaterialManager::createMaterial(renderResource, matDesc);
    }
    bool createTexture(const std::string& name, const std::wstring& filePath)
    {
        return TextureManager::loadTexture(
			renderResource, device, commandList, name, filePath);
	}

    MeshGeometry* getMeshGeometry(const std::string& name)const
    {
		return renderResource.getMeshGeometry(name);
    }
    Material* getMaterial(const std::string& name)const
    {
		return renderResource.getMaterial(name);
    }
    Texture* getTexture(const std::string& name)const
    {
		return renderResource.getTexture(name);
    }

private:
    RenderResource renderResource;
    ID3D12Device* device = nullptr;
    ID3D12GraphicsCommandList* commandList = nullptr;
};

class RenderResource
{
	friend class GeometryManager;
	friend class MaterialManager;
	friend class TextureManager;
public:
    RenderResource() = default;
	~RenderResource() = default;

protected:
    bool addGeometry(std::unique_ptr<MeshGeometry> mesh)
    {
        meshes[mesh->name] = std::move(mesh);
        return true;
	}
    bool addMaterial(std::unique_ptr<Material> material)
    {
        materials[material->name] = std::move(material);
        return true;
    }
    bool addTexture(std::unique_ptr<Texture> texture)
    {
        textures[texture->name] = std::move(texture);
        return true;
	}

public:
    MeshGeometry* getMeshGeometry(const std::string& name)const
    {
        auto it = meshes.find(name);
        if (it != meshes.end())
            return it->second.get();

        return nullptr;
	}
	Material* getMaterial(const std::string& name)const
    {
        auto it = materials.find(name);
        if (it != materials.end())
            return it->second.get();

        return nullptr;
    }
    Texture* getTexture(const std::string& name)const
    {
        auto it = textures.find(name);
        if (it != textures.end())
            return it->second.get();

        return nullptr;
	}
private:
    std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> meshes;				// 储存几何网格体的无序图
    std::unordered_map<std::string, std::unique_ptr<Material>> materials;				// 存储材质的无序图
    std::unordered_map<std::string, std::unique_ptr<Texture>> textures;					// 存储纹理的无序图
};


// 几何体类型枚举
enum class GeometryType
{
    Cylinder,
    Sphere,
    Grid,
    ObjModel,
};
// 几何体创建参数结构体
struct SubmeshGeometryParams
{
    // 默认构造函数
    SubmeshGeometryParams() = default;

    GeometryType type;
    std::string name;

    // 通用参数
    union
    {
        // 圆柱体参数
        struct
        {
            float bottomRadius;
            float topRadius;
            float height;
            uint32_t sliceCount;
            uint32_t stackCount;
        } cylinder;

        // 球体参数
        struct
        {
            float radius;
            uint32_t sliceCount;
            uint32_t stackCount;
        } sphere;

        // 网格参数
        struct
        {
            float width;
            float depth;
            uint32_t m;
            uint32_t n;
        } grid;
    };

    // OBJ文件路径
    std::wstring objFilePath;

    static SubmeshGeometryParams Cylinder(
        const std::string& name,
        float bottomRadius,
        float topRadius,
        float height,
        uint32_t sliceCount,
        uint32_t stackCount)
    {
        SubmeshGeometryParams params;
        params.type = GeometryType::Cylinder;
        params.name = name;
        params.cylinder.bottomRadius = bottomRadius;
        params.cylinder.topRadius = topRadius;
        params.cylinder.height = height;
        params.cylinder.sliceCount = sliceCount;
        params.cylinder.stackCount = stackCount;
        return params;
    }

    static SubmeshGeometryParams Sphere(
        const std::string& name,
        float radius,
        uint32_t sliceCount,
        uint32_t stackCount)
    {
        SubmeshGeometryParams params;
        params.type = GeometryType::Sphere;
        params.name = name;
        params.sphere.radius = radius;
        params.sphere.sliceCount = sliceCount;
        params.sphere.stackCount = stackCount;
        return params;
    }

    static SubmeshGeometryParams Grid(
        const std::string& name,
        float width,
        float depth,
        uint32_t m,
        uint32_t n)
    {
        SubmeshGeometryParams params;
        params.type = GeometryType::Grid;
        params.name = name;
        params.grid.width = width;
        params.grid.depth = depth;
        params.grid.m = m;
        params.grid.n = n;
        return params;
    }

    static SubmeshGeometryParams ImportedOBJ(
        const std::string& name,
        const std::wstring& filePath)
    {
        SubmeshGeometryParams params;
        params.type = GeometryType::ObjModel;
        params.name = name;
        params.objFilePath = filePath;
        return params;
    }
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
        const bool isUse32BitIndices,
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


struct MaterialDesc
{
    std::string name;

    DirectX::XMFLOAT4 albedo = { 1.0f,1.0f,1.0f,1.0f }; // 漫反射反照率
    float metallic = 0.0f;								// 金属度
    float roughness = 0.0f;                             // 材质粗糙度
    float ior = 1.0f;									// 折射率
    DirectX::XMFLOAT3 emissive = { 0,0,0 };             // 自发光
};
class MaterialManager
{
    friend class RenderResourceManager;
protected:
    static bool createMaterial(
        RenderResource& resource,
        const MaterialDesc& matDesc);
};


class TextureManager
{
	friend class RenderResourceManager;
protected:
    static bool TextureManager::loadTexture(
        RenderResource& resource,
        ID3D12Device* device,
        ID3D12GraphicsCommandList* commandList,
        const std::string& name,
        const std::wstring& filePath);
};