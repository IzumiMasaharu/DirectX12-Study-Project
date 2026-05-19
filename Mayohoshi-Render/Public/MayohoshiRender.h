#pragma once

#include "MayohoshiTypes.h"

#include <memory>

namespace Mayohoshi
{
    class MayohoshiRender
    {
    public:
        explicit MayohoshiRender(HINSTANCE instance = nullptr);
        ~MayohoshiRender();

        MayohoshiRender(const MayohoshiRender&) = delete;
        MayohoshiRender& operator=(const MayohoshiRender&) = delete;

        bool Initialize(const RendererDesc& desc);
        bool AttachToWindow(HWND hwnd, UINT width = 0, UINT height = 0);
        bool CreateRenderWindow(UINT width = 1280, UINT height = 720, const std::wstring& title = L"Mayohoshi Render");
        int RunMessageLoop() const;
        void Stop();
        const std::wstring& GetLastErrorMessage() const;

        CanvasHandle CreateCanvas(const CanvasDesc& desc);
        CanvasHandle CreateCanvas(const std::string& name, UINT width = 0, UINT height = 0);
        TextureHandle CreateTexture(const TextureDesc& desc);
        TextureHandle CreateTexture(const std::string& name, const std::wstring& filePath);
        TextureSetHandle BindTextures(const TextureSetDesc& desc);
        TextureSetHandle BindTextures(
            const std::string& name,
            const TextureHandle& diffuse,
            const TextureHandle& normal = {},
            const TextureHandle& depth = {});
        MaterialHandle CreateMaterial(const MaterialDesc& desc);
        MaterialHandle CreateMaterial(
            const std::string& name,
            const DirectX::XMFLOAT4& albedo = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
            float roughness = 0.5f,
            float metallic = 0.0f,
            RenderLayer renderLayer = RenderLayers::Opaque);
        ModelHandle CreateCylinder(const CylinderDesc& desc);
        ModelHandle CreateCylinder(
            const std::string& name,
            float bottomRadius,
            float topRadius,
            float height,
            uint32_t sliceCount = 32,
            uint32_t stackCount = 8,
            RenderLayer layer = RenderLayers::Opaque);
        ModelHandle CreateSphere(const SphereDesc& desc);
        ModelHandle CreateSphere(
            const std::string& name,
            float radius,
            uint32_t sliceCount = 32,
            uint32_t stackCount = 16,
            RenderLayer layer = RenderLayers::Opaque);
        ModelHandle CreateGrid(const GridDesc& desc);
        ModelHandle CreateGrid(
            const std::string& name,
            float width,
            float depth,
            uint32_t m = 2,
            uint32_t n = 2,
            RenderLayer layer = RenderLayers::Opaque);
        ModelHandle CreateModel(const ObjModelDesc& desc);
        ModelHandle CreateModel(
            const std::string& name,
            const std::wstring& filePath,
            RenderLayer layer = RenderLayers::Opaque,
            bool use32BitIndices = true);
        RenderObjectHandle AddInstance(const DrawDesc& desc);
        RenderObjectHandle AddInstance(
            const std::string& name,
            const ModelHandle& model,
            const MaterialHandle& material,
            const TextureSetHandle& textures,
            const TransformDesc& transform = {},
            TextureFlags textureFlags = ToTextureFlags(TextureType::TEX_DIFFUSE));
        RenderObjectHandle AddInstances(const InstanceBatchDesc& desc);
        RenderObjectHandle AddInstances(
            const std::string& name,
            const ModelHandle& model,
            const std::vector<RenderInstanceDesc>& instances);
        RenderObjectHandle AddToRender(const DrawDesc& desc);
        void DrawTo(const CanvasHandle& canvas);
        void DrawToScreen(const CanvasHandle& canvas);

        void SetAmbientLight(const DirectX::XMFLOAT3& color, float intensity = 1.0f);
        LightHandle AddLight(const LightDesc& desc);
        LightHandle AddDirectionalLight(
            const DirectX::XMFLOAT3& direction,
            const DirectX::XMFLOAT3& intensity = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f),
            const std::string& name = {});
        LightHandle AddPointLight(
            const DirectX::XMFLOAT3& position,
            const DirectX::XMFLOAT3& intensity = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f),
            float range = 10.0f,
            const std::string& name = {});
        LightHandle AddSpotLight(
            const DirectX::XMFLOAT3& position,
            const DirectX::XMFLOAT3& direction,
            const DirectX::XMFLOAT3& intensity = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f),
            float range = 10.0f,
            float spotPower = 64.0f,
            const std::string& name = {});
        bool SetLight(const LightHandle& light, const LightDesc& desc);
        bool SetLightPosition(const LightHandle& light, const DirectX::XMFLOAT3& position);
        bool SetLightDirection(const LightHandle& light, const DirectX::XMFLOAT3& direction);
        bool SetLightIntensity(const LightHandle& light, const DirectX::XMFLOAT3& intensity);
        void ClearLights();
        void SetUpdateCallback(UpdateCallback callback);

        bool SetObjectTransform(const RenderObjectHandle& object, const TransformDesc& transform, size_t instanceIndex = 0);
        void SetCameraPosition(const DirectX::XMFLOAT3& position);
        void LookAt(const DirectX::XMFLOAT3& target);
        void SetWireframe(bool enabled);
        void Resize(UINT width, UINT height);

    private:
        class Impl;
        std::unique_ptr<Impl> impl;
    };
}
