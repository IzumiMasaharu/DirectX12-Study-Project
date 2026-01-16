#include "PipelineResourceManager.h"
#include "DxUtil.h"

// ========== PipelineResourceManager Implement ==========
PipelineResourceManager::PipelineResourceManager(ID3D12Device* device)
    : device(device), rootSig(nullptr) { }

void PipelineResourceManager::setRootSignature(ID3D12RootSignature* rootSignature)
{
    this->rootSig = rootSignature;
}

bool PipelineResourceManager::buildShader(
    const std::string& name,
    const std::string& shaderFileName,
    const std::string& entryPoint)
{
    return PipelineRegistry::registerShader(pipelineResource, name, shaderFileName, entryPoint);
}

bool PipelineResourceManager::buildInputLayout(
    const std::string& name,
    const std::vector<D3D12_INPUT_ELEMENT_DESC>& layout)
{
    return !PipelineRegistry::registerInputLayout(pipelineResource, name, layout).empty();
}

bool PipelineResourceManager::buildPSO(
    const PSODesc& desc,
    const std::string& name)
{
    return PipelineRegistry::registerPSO(pipelineResource, name, device, rootSig, desc);
}

ID3D12PipelineState* PipelineResourceManager::getPSO(const std::string& name) const
{
    return pipelineResource.getPSO(name);
}

// ========== PipelineResource Implement ==========
ID3DBlob* PipelineResource::getShader(const std::string& name) const
{
    return shaders.at(name).Get();
}

std::vector<D3D12_INPUT_ELEMENT_DESC> PipelineResource::getInputLayout(const std::string& name) const
{
    return inputLayout.at(name);
}

ID3D12PipelineState* PipelineResource::getPSO(const std::string& name) const
{
    return PSOs.at(name).Get();
}

Microsoft::WRL::ComPtr<ID3DBlob> PipelineResource::addShader(
    const std::string& name,
    Microsoft::WRL::ComPtr<ID3DBlob> shader)
{
    shaders[name] = shader;
    return shaders[name];
}

std::vector<D3D12_INPUT_ELEMENT_DESC>& PipelineResource::addInputLayout(
    const std::string& name,
    const std::vector<D3D12_INPUT_ELEMENT_DESC>& layout)
{
    inputLayout[name] = layout;
    return inputLayout[name];
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineResource::addPSO(
    const std::string& name,
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso)
{
    PSOs[name] = pso;
    return PSOs[name];
}

// ========== PipelineRegistry Implement ==========
Microsoft::WRL::ComPtr<ID3DBlob> PipelineRegistry::registerShader(
    PipelineResource& resource,
    const std::string& name,
    const std::string& shaderFileName,
    const std::string& entryPoint)
{
    Microsoft::WRL::ComPtr<ID3DBlob> shaderBlob;
    shaderBlob = DxUtil::CompileShaderOnline(AnsiToWstring(shaderFileName), nullptr, entryPoint, "vs_5_1");
    return resource.addShader(name, shaderBlob);
}

std::vector<D3D12_INPUT_ELEMENT_DESC>& PipelineRegistry::registerInputLayout(
    PipelineResource& resource,
    const std::string& name,
    const std::vector<D3D12_INPUT_ELEMENT_DESC>& layout)
{
    return resource.addInputLayout(name, layout);
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineRegistry::registerPSO(
    PipelineResource& resource,
    const std::string& name,
    ID3D12Device* device,
    ID3D12RootSignature* rootSig,
    const PSODesc& desc)
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
    ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    psoDesc.pRootSignature = rootSig;

    psoDesc.VS =
    {
        reinterpret_cast<BYTE*>(resource.getShader(desc.vs)->GetBufferPointer()),
        resource.getShader(desc.vs)->GetBufferSize()
    };
    psoDesc.PS =
    {
        reinterpret_cast<BYTE*>(resource.getShader(desc.ps)->GetBufferPointer()),
        resource.getShader(desc.ps)->GetBufferSize()
    };
    psoDesc.InputLayout = {
        resource.getInputLayout(desc.inputLayout).data(),
        (UINT)resource.getInputLayout(desc.inputLayout).size()
    };
    psoDesc.RasterizerState = desc.rasterizerState;
    psoDesc.BlendState = desc.blendState;
    psoDesc.DepthStencilState = desc.stencilState;
    psoDesc.PrimitiveTopologyType = desc.primitiveTopologyType;

    psoDesc.SampleMask = UINT_MAX;
    psoDesc.NumRenderTargets = desc.numRenderTargets;
    std::copy(desc.rtvFormats.begin(), desc.rtvFormats.end(), psoDesc.RTVFormats);
    psoDesc.DSVFormat = desc.dsvFormat;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;
    psoDesc.NodeMask = 0;
    psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
    ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));

    return resource.addPSO(name, pso);
}