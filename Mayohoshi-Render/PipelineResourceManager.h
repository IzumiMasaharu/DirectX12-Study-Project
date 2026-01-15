#pragma once

#pragma once

#include "DxUtil.h"
#include <d3d12.h>
#include <wrl.h>
#include <unordered_map>
#include <vector>
#include <array>
#include <string>

#include "d3dx12.h"

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

    std::array<DXGI_FORMAT, 8> rtvFormats = { DXGI_FORMAT_R8G8B8A8_UNORM }; // 8的来源：D3D12_GRAPHICS_PIPELINE_STATE_DESC::DXGI_FORMAT RTVFormats[ 8 ];
    DXGI_FORMAT dsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
};

class PipelineResourceManager
{
public:
    explicit PipelineResourceManager(ID3D12Device* device): device(device) {}
    ~PipelineResourceManager() = default;
public:
    void setRootSignature(ID3D12RootSignature* rootSignature)
    {
        this->rootSig = rootSignature;
	}

    bool buildShader(
        const std::string& name,
        const std::string& shaderFileName,
        const std::string& entryPoint)
    {
        return PipelineRegistry::registerShader(pipelineResource, name, shaderFileName, entryPoint);
	}

    bool buildInputLayout(
        const std::string& name,
        const std::vector<D3D12_INPUT_ELEMENT_DESC>& layout)
    {
        return !PipelineRegistry::registerInputLayout(pipelineResource, name, layout).empty();
    }

    bool buildPSO(
        const PSODesc& desc,
        const std::string& name)
    {
		return PipelineRegistry::registerPSO(pipelineResource, name, device, rootSig, desc);
    }

    ID3D12PipelineState* getPSO(const std::string& name) const
    {
        return pipelineResource.getPSO(name);
    }

private:
    ID3D12Device* device;
    ID3D12RootSignature* rootSig;

    PipelineResource pipelineResource;
};

class PipelineResource
{
	friend class PipelineRegistry;

public:
    PipelineResource() = default;
    ~PipelineResource() = default;

    ID3DBlob* getShader(const std::string& name) const
    {
        return shaders.at(name).Get();
    }
    std::vector<D3D12_INPUT_ELEMENT_DESC> getInputLayout(const std::string& name) const
    {
        return inputLayout.at(name);
    }
    ID3D12PipelineState* getPSO(const std::string& name) const
    {
        return PSOs.at(name).Get();
    }
protected:
    Microsoft::WRL::ComPtr<ID3DBlob> addShader(const std::string& name, Microsoft::WRL::ComPtr<ID3DBlob> shader)
    {
        shaders[name] = shader;
		return shaders[name];
    }
    std::vector<D3D12_INPUT_ELEMENT_DESC>& addInputLayout(const std::string& name, const std::vector<D3D12_INPUT_ELEMENT_DESC>& layout)
    {
        inputLayout[name] = layout;
		return inputLayout[name];
	}
    Microsoft::WRL::ComPtr<ID3D12PipelineState> addPSO(const std::string& name, Microsoft::WRL::ComPtr<ID3D12PipelineState> pso)
    {
        PSOs[name] = pso;
		return PSOs[name];
	}
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
        const std::string& entryPoint)
    {
		Microsoft::WRL::ComPtr<ID3DBlob> shaderBlob;
        shaderBlob = DxUtil::CompileShaderOnline(AnsiToWstring(shaderFileName), nullptr, entryPoint, "vs_5_1");
		return resource.addShader(name,shaderBlob);
    }

    // TODO:以后可改成链式定义输入布局
    static std::vector<D3D12_INPUT_ELEMENT_DESC>& registerInputLayout(
        PipelineResource& resource,
        const std::string& name,
        const std::vector<D3D12_INPUT_ELEMENT_DESC>& layout)
    {
		return resource.addInputLayout(name, layout);
    };

    static Microsoft::WRL::ComPtr<ID3D12PipelineState> registerPSO(
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
        psoDesc.InputLayout = { resource.getInputLayout(desc.inputLayout).data(), (UINT)resource.getInputLayout(desc.inputLayout).size() };
        psoDesc.RasterizerState = desc.rasterizerState;
        psoDesc.BlendState = desc.blendState;
        psoDesc.DepthStencilState = desc.stencilState;
        psoDesc.PrimitiveTopologyType = desc.primitiveTopologyType;

        psoDesc.SampleMask = UINT_MAX;
        psoDesc.NumRenderTargets = 1;
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
};