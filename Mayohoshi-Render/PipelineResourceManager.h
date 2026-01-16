#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <unordered_map>
#include <vector>
#include <array>
#include <string>
#include "d3dx12.h"

class PipelineResource;
class PipelineRegistry;

class PipelineResourceManager
{
public:
    explicit PipelineResourceManager(ID3D12Device* device);
    PipelineResourceManager() = delete;
    ~PipelineResourceManager() = default;

public:
    void setRootSignature(ID3D12RootSignature* rootSignature);

    bool buildShader(
        const std::string& name,
        const std::string& shaderFileName,
        const std::string& entryPoint);

    bool buildInputLayout(
        const std::string& name,
        const std::vector<D3D12_INPUT_ELEMENT_DESC>& layout);

    bool buildPSO(
        const PSODesc& desc,
        const std::string& name);

    ID3D12PipelineState* getPSO(const std::string& name) const;

private:
    ID3D12Device* device;
    ID3D12RootSignature* rootSig;
    PipelineResource pipelineResource;
};

struct PSODesc
{
    std::string vs;
    std::string ps;
    std::string inputLayout;

    D3D12_RASTERIZER_DESC rasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    D3D12_BLEND_DESC blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    D3D12_DEPTH_STENCIL_DESC stencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    UINT numRenderTargets = 1;

    std::array<DXGI_FORMAT, 8> rtvFormats = { DXGI_FORMAT_R8G8B8A8_UNORM };
    DXGI_FORMAT dsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
};

class PipelineResource
{
    friend class PipelineRegistry;
public:
    PipelineResource() = default;
    ~PipelineResource() = default;

    ID3DBlob* getShader(const std::string& name) const;
    std::vector<D3D12_INPUT_ELEMENT_DESC> getInputLayout(const std::string& name) const;
    ID3D12PipelineState* getPSO(const std::string& name) const;

protected:
    Microsoft::WRL::ComPtr<ID3DBlob> addShader(
        const std::string& name,
        Microsoft::WRL::ComPtr<ID3DBlob> shader);

    std::vector<D3D12_INPUT_ELEMENT_DESC>& addInputLayout(
        const std::string& name,
        const std::vector<D3D12_INPUT_ELEMENT_DESC>& layout);

    Microsoft::WRL::ComPtr<ID3D12PipelineState> addPSO(
        const std::string& name,
        Microsoft::WRL::ComPtr<ID3D12PipelineState> pso);

private:
    std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> shaders;
    std::unordered_map<std::string, std::vector<D3D12_INPUT_ELEMENT_DESC>> inputLayout;
    std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> PSOs;
};


class PipelineRegistry
{
    friend class PipelineResourceManager;

protected:
    static Microsoft::WRL::ComPtr<ID3DBlob> registerShader(
        PipelineResource& resource,
        const std::string& name,
        const std::string& shaderFileName,
        const std::string& entryPoint);

    static std::vector<D3D12_INPUT_ELEMENT_DESC>& registerInputLayout(
        PipelineResource& resource,
        const std::string& name,
        const std::vector<D3D12_INPUT_ELEMENT_DESC>& layout);

    static Microsoft::WRL::ComPtr<ID3D12PipelineState> registerPSO(
        PipelineResource& resource,
        const std::string& name,
        ID3D12Device* device,
        ID3D12RootSignature* rootSig,
        const PSODesc& desc);
};