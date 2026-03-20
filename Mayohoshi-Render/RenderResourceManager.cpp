#include "RenderResourceManager.h"
#include "ShaderResource.h"

// ========== GeometryManager Implement ==========
bool GeometryManager::createMeshGeometry(
    RenderResource& resource,
    ID3D12Device* device,
    ID3D12GraphicsCommandList* commandList,
    const std::string& meshName,
    const bool isUse32BitIndices,
    const std::vector<SubmeshGeometryParams>& submeshParams)
{
    if (isUse32BitIndices) {
        return createMeshGeometryImpl<uint32_t>(
            resource, device, commandList, meshName, submeshParams);
    }
    else {
        return createMeshGeometryImpl<uint16_t>(
            resource, device, commandList, meshName, submeshParams);
    }
}

template<typename IndexType>
bool GeometryManager::createMeshGeometryImpl(
    RenderResource& resource,
    ID3D12Device* device,
    ID3D12GraphicsCommandList* commandList,
    const std::string& meshName,
    const std::vector<SubmeshGeometryParams>& submeshParams)
{
    constexpr bool is32Bit = std::is_same_v<IndexType, uint32_t>;

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
        const auto& meshData = allMeshData.back();

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
        const auto& meshData = allMeshData[paramIdx];

        SubmeshGeometry submesh;
        submesh.name = params.name;
        submesh.vertexBaseLocation = vertexOffset;
        submesh.indexStartLocation = indexOffset;
        submesh.indexCount = static_cast<UINT>(is32Bit ? meshData.getIndices32().size() : meshData.getIndices16().size());

        for (const auto& vertex : meshData.vertices)
            vertices.emplace_back({vertex.position,vertex.normal,vertex.tangent,vertex.textureUV});

        if (is32Bit) 
            indices.insert(indices.end(), meshData.getIndices32().begin(), meshData.getIndices32().end());
        else 
            indices.insert(indices.end(), meshData.getIndices16().begin(), meshData.getIndices16().end());

        vertexOffset += static_cast<UINT>(meshData.vertices.size());
        indexOffset += submesh.indexCount;

        mesh->submeshList.emplace(submesh.name, std::move(submesh));
    }

    const UINT vertexBufferByteSize = static_cast<UINT>(vertices.size() * sizeof(VertexConstants));
    const UINT indexBufferByteSize = static_cast<UINT>(indices.size() * sizeof(IndexType));

    ThrowIfFailed(D3DCreateBlob(vertexBufferByteSize, &mesh->vertexBufferCPU));
    CopyMemory(mesh->vertexBufferCPU->GetBufferPointer(), vertices.data(), vertexBufferByteSize);

    ThrowIfFailed(D3DCreateBlob(indexBufferByteSize, &mesh->indexBufferCPU));
    CopyMemory(mesh->indexBufferCPU->GetBufferPointer(), indices.data(), indexBufferByteSize);

    mesh->vertexBufferGPU = DxUtil::CreateDefaultBuffer(
        device, commandList, vertices.data(), vertexBufferByteSize, mesh->vertexBufferUploader);
    mesh->indexBufferGPU = DxUtil::CreateDefaultBuffer(
        device, commandList, indices.data(), indexBufferByteSize, mesh->indexBufferUploader);

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


// ========== MaterialManager Implement ==========
bool MaterialManager::createMaterial(
    RenderResource& resource,
    const MaterialDesc& matDesc)
{
    static UINT materialIndex = 0;

    auto mat = std::make_unique<Material>();
    mat->name = matDesc.name;
    mat->albedo = matDesc.albedo;
	mat->opacity = matDesc.opacity;
    mat->metallic = matDesc.metallic;
    mat->roughness = matDesc.roughness;
    mat->ior = matDesc.ior;
    mat->materialIndex = materialIndex++;
    mat->numDirtyFrames = gNumFrameResources;

    return resource.addMaterial(std::move(mat));
}


// ========== TextureManager Implement ==========
bool TextureManager::loadTexture(
    RenderResource& resource,
    ID3D12Device* device,
    ID3D12GraphicsCommandList* commandList,
    const std::string& name,
    const std::wstring& filePath)
{
    static UINT srvIndex = 0;

    auto tex = std::make_unique<Texture>();
    tex->name = name;
    tex->srvHeapIndex = srvIndex++;

    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(device, commandList, filePath.c_str(), tex->resource, tex->uploadHeap));

    return resource.addTexture(std::move(tex));
}