#include "RenderResourceManager.h"
#include "ShaderResource.h"

#include <type_traits>

using namespace DirectX;

SubmeshGeometryParams SubmeshGeometryParams::Cylinder(
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

SubmeshGeometryParams SubmeshGeometryParams::Sphere(
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

SubmeshGeometryParams SubmeshGeometryParams::Grid(
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

SubmeshGeometryParams SubmeshGeometryParams::ImportedOBJ(
    const std::string& name,
    const std::wstring& filePath)
{
    SubmeshGeometryParams params;
    params.type = GeometryType::ObjModel;
    params.name = name;
    params.objFilePath = filePath;
    return params;
}

MeshGeometry* RenderResource::getMeshGeometry(const std::string& name) const
{
    auto it = meshes.find(name);
    return it == meshes.end() ? nullptr : it->second.get();
}

Material* RenderResource::getMaterial(const std::string& name) const
{
    auto it = materials.find(name);
    return it == materials.end() ? nullptr : it->second.get();
}

Texture* RenderResource::getTexture(const std::string& name) const
{
    auto it = textures.find(name);
    return it == textures.end() ? nullptr : it->second.get();
}

const TextureTable* RenderResource::getTextureTable(const std::string& name) const
{
    auto it = textureTableNameToIndex.find(name);
    if (it == textureTableNameToIndex.end() || it->second >= textureTables.size())
        return nullptr;
    return &textureTables[it->second];
}

UINT RenderResource::getTextureTableIndex(const std::string& name) const
{
    auto it = textureTableNameToIndex.find(name);
    return it == textureTableNameToIndex.end() ? 0 : it->second;
}

void RenderResource::forEachTexture(const std::function<void(const Texture&)>& callback) const
{
    for (const auto& it : textures)
        callback(*it.second);
}

void RenderResource::forEachMaterial(const std::function<void(Material*)>& callback) const
{
    for (const auto& it : materials)
        callback(it.second.get());
}

void RenderResource::forEachTextureTable(const std::function<void(UINT, const TextureTable&)>& callback) const
{
    for (UINT i = 0; i < static_cast<UINT>(textureTables.size()); ++i)
        callback(i, textureTables[i]);
}

bool RenderResource::addGeometry(std::unique_ptr<MeshGeometry> mesh)
{
    if (!mesh || mesh->name.empty() || getMeshGeometry(mesh->name))
        return false;
    meshes.emplace(mesh->name, std::move(mesh));
    return true;
}

bool RenderResource::addMaterial(std::unique_ptr<Material> material)
{
    if (!material || material->name.empty() || getMaterial(material->name))
        return false;
    materials.emplace(material->name, std::move(material));
    return true;
}

bool RenderResource::addTexture(std::unique_ptr<Texture> texture)
{
    if (!texture || texture->name.empty() || getTexture(texture->name))
        return false;
    textures.emplace(texture->name, std::move(texture));
    return true;
}

bool RenderResource::addTextureTable(const std::string& name, const TextureTable& table)
{
    if (name.empty() || textureTableNameToIndex.find(name) != textureTableNameToIndex.end())
        return false;

    const UINT index = allocateTextureTableIndex();
    if (index != textureTables.size())
        return false;

    textureTables.emplace_back(table);
    textureTableNameToIndex.emplace(name, index);
    return true;
}

void RenderResourceManager::initialize(ID3D12Device* inDevice, ID3D12GraphicsCommandList* cmdList)
{
    device = inDevice;
    commandList = cmdList;
}

bool RenderResourceManager::createMeshGeometry(
    const std::string& name,
    bool isUse32BitIndices,
    const std::vector<SubmeshGeometryParams>& params)
{
    return GeometryManager::createMeshGeometry(renderResource, device, commandList, name, isUse32BitIndices, params);
}

bool RenderResourceManager::createMaterial(const MaterialDesc& matDesc)
{
    return MaterialManager::createMaterial(renderResource, matDesc);
}

bool RenderResourceManager::createTexture(const std::string& name, const std::wstring& filePath)
{
    return TextureManager::loadTexture(renderResource, device, commandList, name, filePath);
}

bool RenderResourceManager::createTextureTable(const TextureTableDesc& desc)
{
    return TextureTableManager::createTextureTable(renderResource, desc);
}

bool GeometryManager::createMeshGeometry(
    RenderResource& resource,
    ID3D12Device* device,
    ID3D12GraphicsCommandList* commandList,
    const std::string& meshName,
    bool isUse32BitIndices,
    const std::vector<SubmeshGeometryParams>& submeshParams)
{
    if (!device || !commandList || meshName.empty() || submeshParams.empty())
        return false;

    if (isUse32BitIndices)
        return createMeshGeometryImpl<uint32_t>(resource, device, commandList, meshName, submeshParams);

    return createMeshGeometryImpl<uint16_t>(resource, device, commandList, meshName, submeshParams);
}

template<typename IndexType>
bool GeometryManager::createMeshGeometryImpl(
    RenderResource& resource,
    ID3D12Device* device,
    ID3D12GraphicsCommandList* commandList,
    const std::string& meshName,
    const std::vector<SubmeshGeometryParams>& submeshParams)
{
    constexpr bool is32Bit = std::is_same<IndexType, uint32_t>::value;

    if (resource.getMeshGeometry(meshName))
        return false;

    auto mesh = std::make_unique<MeshGeometry>();
    mesh->name = meshName;

    std::vector<GeometryGenerator::MeshData> allMeshData;
    allMeshData.reserve(submeshParams.size());

    size_t totalVertices = 0;
    size_t totalIndices = 0;

    for (const auto& params : submeshParams)
    {
        allMeshData.emplace_back(buildMeshData(params));
        auto& meshData = allMeshData.back();

        totalVertices += meshData.vertices.size();
        totalIndices += is32Bit ? meshData.getIndices32().size() : meshData.getIndices16().size();
    }

    std::vector<VertexConstants> vertices;
    std::vector<IndexType> indices;
    vertices.reserve(totalVertices);
    indices.reserve(totalIndices);

    UINT vertexOffset = 0;
    UINT indexOffset = 0;

    for (size_t paramIdx = 0; paramIdx < submeshParams.size(); ++paramIdx)
    {
        const auto& params = submeshParams[paramIdx];
        auto& meshData = allMeshData[paramIdx];

        SubmeshGeometry submesh;
        submesh.name = params.name;
        submesh.vertexBaseLocation = vertexOffset;
        submesh.indexStartLocation = indexOffset;
        submesh.indexCount = static_cast<UINT>(is32Bit ? meshData.getIndices32().size() : meshData.getIndices16().size());

        for (const auto& vertex : meshData.vertices)
            vertices.push_back({ vertex.position, vertex.normal, vertex.tangent, vertex.textureUV });

        if (is32Bit)
        {
            const auto& source = meshData.getIndices32();
            indices.insert(indices.end(), source.begin(), source.end());
        }
        else
        {
            const auto& source = meshData.getIndices16();
            indices.insert(indices.end(), source.begin(), source.end());
        }

        vertexOffset += static_cast<UINT>(meshData.vertices.size());
        indexOffset += submesh.indexCount;

        mesh->submeshList.emplace(submesh.name, submesh);
    }

    const UINT vertexBufferByteSize = static_cast<UINT>(vertices.size() * sizeof(VertexConstants));
    const UINT indexBufferByteSize = static_cast<UINT>(indices.size() * sizeof(IndexType));

    ThrowIfFailed(D3DCreateBlob(vertexBufferByteSize, &mesh->vertexBufferCPU));
    CopyMemory(mesh->vertexBufferCPU->GetBufferPointer(), vertices.data(), vertexBufferByteSize);

    ThrowIfFailed(D3DCreateBlob(indexBufferByteSize, &mesh->indexBufferCPU));
    CopyMemory(mesh->indexBufferCPU->GetBufferPointer(), indices.data(), indexBufferByteSize);

    mesh->vertexBufferGPU = D3D12Utility::CreateDefaultBuffer(device, commandList, vertices.data(), vertexBufferByteSize, mesh->vertexBufferUploader);
    mesh->indexBufferGPU = D3D12Utility::CreateDefaultBuffer(device, commandList, indices.data(), indexBufferByteSize, mesh->indexBufferUploader);

    mesh->vertexByteStride = sizeof(VertexConstants);
    mesh->vertexBufferByteSize = vertexBufferByteSize;
    mesh->indexFormat = is32Bit ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
    mesh->indexBufferByteSize = indexBufferByteSize;

    return resource.addGeometry(std::move(mesh));
}

GeometryGenerator::MeshData GeometryManager::buildMeshData(const SubmeshGeometryParams& params)
{
    switch (params.type)
    {
    case GeometryType::Cylinder:
        return GeometryGenerator::CreateCylinder(
            params.cylinder.bottomRadius,
            params.cylinder.topRadius,
            params.cylinder.height,
            params.cylinder.sliceCount,
            params.cylinder.stackCount);

    case GeometryType::Sphere:
        return GeometryGenerator::CreateBall(
            params.sphere.radius,
            params.sphere.sliceCount,
            params.sphere.stackCount);

    case GeometryType::Grid:
        return GeometryGenerator::CreateGird(
            params.grid.width,
            params.grid.depth,
            params.grid.m,
            params.grid.n);

    case GeometryType::ObjModel:
        return GeometryGenerator::CreateImportedGeometryFromOBJ(params.objFilePath);

    default:
        return GeometryGenerator::MeshData();
    }
}

bool MaterialManager::createMaterial(RenderResource& resource, const MaterialDesc& matDesc)
{
    if (matDesc.name.empty() || resource.getMaterial(matDesc.name))
        return false;

    auto mat = std::make_unique<Material>();
    mat->name = matDesc.name;
    mat->albedo = matDesc.albedo;
    mat->metallic = matDesc.metallic;
    mat->roughness = matDesc.roughness;
    mat->ior = matDesc.ior;
    mat->emissive = matDesc.emissive;
    mat->materialTransform = matDesc.materialTransform;
    mat->renderLayer = matDesc.renderLayer;
    mat->materialIndex = resource.allocateMaterialIndex();
    mat->numDirtyFrames = gNumFrameResources;

    return resource.addMaterial(std::move(mat));
}

bool TextureManager::loadTexture(
    RenderResource& resource,
    ID3D12Device* device,
    ID3D12GraphicsCommandList* commandList,
    const std::string& name,
    const std::wstring& filePath)
{
    if (name.empty() || filePath.empty() || resource.getTexture(name))
        return false;

    if (!device || !commandList)
    {
        throw D3D12Exception(
            E_POINTER,
            L"Texture load failed because the D3D12 device or command list is not ready.\nTexture: " + AnsiToWstring(name) + L"\nPath: " + filePath,
            AnsiToWstring(__FILE__),
            __LINE__);
    }

    const DWORD attributes = GetFileAttributesW(filePath.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        const HRESULT hr = attributes == INVALID_FILE_ATTRIBUTES ? HRESULT_FROM_WIN32(GetLastError()) : HRESULT_FROM_WIN32(ERROR_DIRECTORY);
        throw D3D12Exception(
            hr,
            L"Texture file is not available.\nTexture: " + AnsiToWstring(name) + L"\nPath: " + filePath,
            AnsiToWstring(__FILE__),
            __LINE__);
    }

    auto tex = std::make_unique<Texture>();
    tex->name = name;
    tex->srvHeapIndex = resource.allocateSrvIndex();

    const HRESULT hr = DirectX::CreateDDSTextureFromFile12(device, commandList, filePath.c_str(), tex->resource, tex->uploadHeap);
    if (FAILED(hr))
    {
        throw D3D12Exception(
            hr,
            L"CreateDDSTextureFromFile12 failed.\nTexture: " + AnsiToWstring(name) + L"\nPath: " + filePath,
            AnsiToWstring(__FILE__),
            __LINE__);
    }

    return resource.addTexture(std::move(tex));
}

bool TextureTableManager::createTextureTable(RenderResource& resource, const TextureTableDesc& desc)
{
    if (desc.name.empty())
        return false;

    TextureTable table;

    if (!desc.diffuseTexture.empty())
    {
        Texture* tex = resource.getTexture(desc.diffuseTexture);
        if (!tex)
            return false;
        table.diffuseIndex = tex->srvHeapIndex;
    }

    if (!desc.normalTexture.empty())
    {
        Texture* tex = resource.getTexture(desc.normalTexture);
        if (!tex)
            return false;
        table.normalIndex = tex->srvHeapIndex;
    }

    if (!desc.depthTexture.empty())
    {
        Texture* tex = resource.getTexture(desc.depthTexture);
        if (!tex)
            return false;
        table.depthIndex = tex->srvHeapIndex;
    }

    return resource.addTextureTable(desc.name, table);
}
