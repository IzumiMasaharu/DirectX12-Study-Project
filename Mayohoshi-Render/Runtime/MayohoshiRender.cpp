#include "MayohoshiRender.h"

#include "RenderRuntime.h"

#include <exception>
#include <functional>
#include <utility>
#include <vector>

namespace Mayohoshi
{
    class MayohoshiRender::Impl
    {
    public:
        using RecordedCommand = std::function<void(RenderRuntime&)>;

        explicit Impl(HINSTANCE inInstance)
            : instance(inInstance ? inInstance : GetModuleHandle(nullptr))
        {
        }

        ~Impl()
        {
            Stop();
        }

        bool Initialize(const RendererDesc& desc)
        {
            if (initialized)
                return true;

            try
            {
                instance = desc.instance ? desc.instance : instance;
                renderer = std::make_unique<RenderRuntime>(instance);
                renderer->SetStartThreads(desc.startThreads);
                renderer->SetShaderDirectory(desc.shaderDirectory);

                if (desc.createOwnWindow)
                    renderer->ConfigureOwnedWindow(desc.width, desc.height, desc.title);
                else
                    renderer->UseExternalWindow(desc.targetWindow, desc.width, desc.height);

                renderer->SetSceneSetup(
                    [this](RenderRuntime& app)
                    {
                        for (const auto& command : recordedCommands)
                            command(app);
                    });

                initialized = true;
                initialized = renderer->Initialize();
                if (initialized)
                    lastErrorMessage.clear();
                return initialized;
            }
            catch (const D3D12Exception& error)
            {
                lastErrorMessage = error.ErrorMessageString();
            }
            catch (const std::exception& error)
            {
                lastErrorMessage = AnsiToWstring(error.what());
            }
            catch (...)
            {
                lastErrorMessage = L"Unknown Mayohoshi renderer initialization error.";
            }

            initialized = false;
            renderer.reset();
            return false;
        }

        bool AttachToWindow(HWND hwnd, UINT width, UINT height)
        {
            RendererDesc desc;
            desc.instance = instance;
            desc.targetWindow = hwnd;
            desc.width = width;
            desc.height = height;
            desc.createOwnWindow = false;
            return Initialize(desc);
        }

        bool CreateRenderWindow(UINT width, UINT height, const std::wstring& title)
        {
            RendererDesc desc;
            desc.instance = instance;
            desc.width = width;
            desc.height = height;
            desc.title = title;
            desc.createOwnWindow = true;
            return Initialize(desc);
        }

        int RunMessageLoop() const
        {
            return renderer ? renderer->RunMessagePump() : -1;
        }

        void Stop()
        {
            if (renderer)
                renderer->Stop();
            initialized = false;
        }

        const std::wstring& GetLastErrorMessage() const
        {
            return lastErrorMessage;
        }

        void Resize(UINT width, UINT height)
        {
            if (initialized && renderer)
                renderer->ResizeRenderTarget(width, height);
        }

        CanvasHandle CreateCanvas(const CanvasDesc& desc)
        {
            CanvasHandle handle{ desc.name.empty() ? "Screen" : desc.name };
            recordOrApply([desc](RenderRuntime& app) { app.CreateCanvas(desc); });
            return handle;
        }

        CanvasHandle CreateCanvas(const std::string& name, UINT width, UINT height)
        {
            return CreateCanvas({ name, width, height });
        }

        TextureHandle CreateTexture(const TextureDesc& desc)
        {
            TextureHandle handle{ desc.name };
            recordOrApplyResource([desc](RenderRuntime& app) { app.CreateTexture(desc); });
            return handle;
        }

        TextureHandle CreateTexture(const std::string& name, const std::wstring& filePath)
        {
            return CreateTexture({ name, filePath });
        }

        TextureSetHandle BindTextures(const TextureSetDesc& desc)
        {
            TextureSetHandle handle{ desc.name };
            recordOrApplyResource([desc](RenderRuntime& app) { app.BindTextures(desc); });
            return handle;
        }

        TextureSetHandle BindTextures(
            const std::string& name,
            const TextureHandle& diffuse,
            const TextureHandle& normal,
            const TextureHandle& depth)
        {
            return BindTextures({ name, diffuse, normal, depth });
        }

        MaterialHandle CreateMaterial(const MaterialDesc& desc)
        {
            MaterialHandle handle{ desc.name };
            recordOrApplyResource([desc](RenderRuntime& app) { app.CreateMaterial(desc); });
            return handle;
        }

        MaterialHandle CreateMaterial(
            const std::string& name,
            const DirectX::XMFLOAT4& albedo,
            float roughness,
            float metallic,
            RenderLayer renderLayer)
        {
            MaterialDesc desc;
            desc.name = name;
            desc.albedo = albedo;
            desc.roughness = roughness;
            desc.metallic = metallic;
            desc.renderLayer = renderLayer;
            return CreateMaterial(desc);
        }

        ModelHandle CreateCylinder(const CylinderDesc& desc)
        {
            ModelHandle handle{ desc.name, desc.name, desc.name + "_Submesh", desc.layer };
            recordOrApplyResource([desc](RenderRuntime& app) { app.CreateCylinder(desc); });
            return handle;
        }

        ModelHandle CreateCylinder(
            const std::string& name,
            float bottomRadius,
            float topRadius,
            float height,
            uint32_t sliceCount,
            uint32_t stackCount,
            RenderLayer layer)
        {
            return CreateCylinder({ name, bottomRadius, topRadius, height, sliceCount, stackCount, layer });
        }

        ModelHandle CreateSphere(const SphereDesc& desc)
        {
            ModelHandle handle{ desc.name, desc.name, desc.name + "_Submesh", desc.layer };
            recordOrApplyResource([desc](RenderRuntime& app) { app.CreateSphere(desc); });
            return handle;
        }

        ModelHandle CreateSphere(
            const std::string& name,
            float radius,
            uint32_t sliceCount,
            uint32_t stackCount,
            RenderLayer layer)
        {
            return CreateSphere({ name, radius, sliceCount, stackCount, layer });
        }

        ModelHandle CreateGrid(const GridDesc& desc)
        {
            ModelHandle handle{ desc.name, desc.name, desc.name + "_Submesh", desc.layer };
            recordOrApplyResource([desc](RenderRuntime& app) { app.CreateGrid(desc); });
            return handle;
        }

        ModelHandle CreateGrid(
            const std::string& name,
            float width,
            float depth,
            uint32_t m,
            uint32_t n,
            RenderLayer layer)
        {
            return CreateGrid({ name, width, depth, m, n, layer });
        }

        ModelHandle CreateModel(const ObjModelDesc& desc)
        {
            ModelHandle handle{ desc.name, desc.name, desc.name + "_Submesh", desc.layer };
            recordOrApplyResource([desc](RenderRuntime& app) { app.CreateModel(desc); });
            return handle;
        }

        ModelHandle CreateModel(
            const std::string& name,
            const std::wstring& filePath,
            RenderLayer layer,
            bool use32BitIndices)
        {
            return CreateModel({ name, filePath, use32BitIndices, layer });
        }

        RenderObjectHandle AddInstance(const DrawDesc& desc)
        {
            RenderObjectHandle handle{ desc.name };
            recordOrApply([desc](RenderRuntime& app) { app.AddInstance(desc); });
            return handle;
        }

        RenderObjectHandle AddInstance(
            const std::string& name,
            const ModelHandle& model,
            const MaterialHandle& material,
            const TextureSetHandle& textures,
            const TransformDesc& transform,
            TextureFlags textureFlags)
        {
            return AddInstance({ name, model, material, textures, transform, textureFlags });
        }

        RenderObjectHandle AddInstances(const InstanceBatchDesc& desc)
        {
            RenderObjectHandle handle{ desc.name };
            recordOrApply([desc](RenderRuntime& app) { app.AddInstances(desc); });
            return handle;
        }

        RenderObjectHandle AddInstances(
            const std::string& name,
            const ModelHandle& model,
            const std::vector<RenderInstanceDesc>& instances)
        {
            return AddInstances({ name, model, instances });
        }

        RenderObjectHandle AddToRender(const DrawDesc& desc)
        {
            return AddInstance(desc);
        }

        void DrawTo(const CanvasHandle& canvas)
        {
            recordOrApply([canvas](RenderRuntime& app) { app.DrawTo(canvas); });
        }

        void DrawToScreen(const CanvasHandle& canvas)
        {
            recordOrApply([canvas](RenderRuntime& app) { app.DrawToScreen(canvas); });
        }

        bool SetObjectTransform(const RenderObjectHandle& object, const TransformDesc& transform, size_t instanceIndex)
        {
            if (initialized)
            {
                try
                {
                    const bool changed = requireRenderer().SetObjectTransform(object, transform, instanceIndex);
                    lastErrorMessage.clear();
                    return changed;
                }
                catch (const D3D12Exception& error)
                {
                    lastErrorMessage = error.ErrorMessageString();
                    return false;
                }
                catch (const std::exception& error)
                {
                    lastErrorMessage = AnsiToWstring(error.what());
                    return false;
                }
                catch (...)
                {
                    lastErrorMessage = L"Unknown Mayohoshi renderer transform error.";
                    return false;
                }
            }

            recordOrApply([object, transform, instanceIndex](RenderRuntime& app)
                {
                    app.SetObjectTransform(object, transform, instanceIndex);
                });
            return true;
        }

        void SetCameraPosition(const DirectX::XMFLOAT3& position)
        {
            recordOrApply([position](RenderRuntime& app) { app.SetCameraPosition(position); });
        }

        void LookAt(const DirectX::XMFLOAT3& target)
        {
            recordOrApply([target](RenderRuntime& app) { app.LookAt(target); });
        }

        void SetWireframe(bool enabled)
        {
            recordOrApply([enabled](RenderRuntime& app) { app.SetWireframe(enabled); });
        }

        void SetAmbientLight(const DirectX::XMFLOAT3& color, float intensity)
        {
            recordOrApply([color, intensity](RenderRuntime& app) { app.SetAmbientLight(color, intensity); });
        }

        LightHandle AddLight(const LightDesc& desc)
        {
            LightHandle handle{ desc.type, nextLightIndex(desc.type), desc.name };
            recordOrApply([desc](RenderRuntime& app) { app.AddLight(desc); });
            return handle;
        }

        LightHandle AddDirectionalLight(
            const DirectX::XMFLOAT3& direction,
            const DirectX::XMFLOAT3& intensity,
            const std::string& name)
        {
            LightDesc desc;
            desc.name = name;
            desc.type = LightType::Directional;
            desc.direction = direction;
            desc.intensity = intensity;
            return AddLight(desc);
        }

        LightHandle AddPointLight(
            const DirectX::XMFLOAT3& position,
            const DirectX::XMFLOAT3& intensity,
            float range,
            const std::string& name)
        {
            LightDesc desc;
            desc.name = name;
            desc.type = LightType::Point;
            desc.position = position;
            desc.intensity = intensity;
            desc.end = range;
            return AddLight(desc);
        }

        LightHandle AddSpotLight(
            const DirectX::XMFLOAT3& position,
            const DirectX::XMFLOAT3& direction,
            const DirectX::XMFLOAT3& intensity,
            float range,
            float spotPower,
            const std::string& name)
        {
            LightDesc desc;
            desc.name = name;
            desc.type = LightType::Spot;
            desc.position = position;
            desc.direction = direction;
            desc.intensity = intensity;
            desc.end = range;
            desc.spotPower = spotPower;
            return AddLight(desc);
        }

        bool SetLight(const LightHandle& light, const LightDesc& desc)
        {
            if (initialized)
            {
                try
                {
                    const bool changed = requireRenderer().SetLight(light, desc);
                    lastErrorMessage.clear();
                    return changed;
                }
                catch (const D3D12Exception& error)
                {
                    lastErrorMessage = error.ErrorMessageString();
                    return false;
                }
                catch (const std::exception& error)
                {
                    lastErrorMessage = AnsiToWstring(error.what());
                    return false;
                }
                catch (...)
                {
                    lastErrorMessage = L"Unknown Mayohoshi renderer light error.";
                    return false;
                }
            }

            recordOrApply([light, desc](RenderRuntime& app) { app.SetLight(light, desc); });
            return true;
        }

        bool SetLightPosition(const LightHandle& light, const DirectX::XMFLOAT3& position)
        {
            if (initialized)
            {
                try
                {
                    const bool changed = requireRenderer().SetLightPosition(light, position);
                    lastErrorMessage.clear();
                    return changed;
                }
                catch (const D3D12Exception& error)
                {
                    lastErrorMessage = error.ErrorMessageString();
                    return false;
                }
                catch (const std::exception& error)
                {
                    lastErrorMessage = AnsiToWstring(error.what());
                    return false;
                }
                catch (...)
                {
                    lastErrorMessage = L"Unknown Mayohoshi renderer light position error.";
                    return false;
                }
            }

            recordOrApply([light, position](RenderRuntime& app) { app.SetLightPosition(light, position); });
            return true;
        }

        bool SetLightDirection(const LightHandle& light, const DirectX::XMFLOAT3& direction)
        {
            if (initialized)
            {
                try
                {
                    const bool changed = requireRenderer().SetLightDirection(light, direction);
                    lastErrorMessage.clear();
                    return changed;
                }
                catch (const D3D12Exception& error)
                {
                    lastErrorMessage = error.ErrorMessageString();
                    return false;
                }
                catch (const std::exception& error)
                {
                    lastErrorMessage = AnsiToWstring(error.what());
                    return false;
                }
                catch (...)
                {
                    lastErrorMessage = L"Unknown Mayohoshi renderer light direction error.";
                    return false;
                }
            }

            recordOrApply([light, direction](RenderRuntime& app) { app.SetLightDirection(light, direction); });
            return true;
        }

        bool SetLightIntensity(const LightHandle& light, const DirectX::XMFLOAT3& intensity)
        {
            if (initialized)
            {
                try
                {
                    const bool changed = requireRenderer().SetLightIntensity(light, intensity);
                    lastErrorMessage.clear();
                    return changed;
                }
                catch (const D3D12Exception& error)
                {
                    lastErrorMessage = error.ErrorMessageString();
                    return false;
                }
                catch (const std::exception& error)
                {
                    lastErrorMessage = AnsiToWstring(error.what());
                    return false;
                }
                catch (...)
                {
                    lastErrorMessage = L"Unknown Mayohoshi renderer light intensity error.";
                    return false;
                }
            }

            recordOrApply([light, intensity](RenderRuntime& app) { app.SetLightIntensity(light, intensity); });
            return true;
        }

        void ClearLights()
        {
            directionalLightCount = 0;
            pointLightCount = 0;
            spotLightCount = 0;
            recordOrApply([](RenderRuntime& app) { app.ClearLights(); });
        }

        void SetUpdateCallback(UpdateCallback callback)
        {
            recordOrApply([callback](RenderRuntime& app) { app.SetUpdateCallback(callback); });
        }

    private:
        uint32_t nextLightIndex(LightType type)
        {
            switch (type)
            {
            case LightType::Directional:
                return directionalLightCount++;
            case LightType::Point:
                return pointLightCount++;
            case LightType::Spot:
                return spotLightCount++;
            }

            return 0;
        }

        void recordOrApply(RecordedCommand command)
        {
            try
            {
                if (initialized)
                    command(requireRenderer());
                else
                    recordedCommands.emplace_back(std::move(command));
                lastErrorMessage.clear();
            }
            catch (const D3D12Exception& error)
            {
                lastErrorMessage = error.ErrorMessageString();
            }
            catch (const std::exception& error)
            {
                lastErrorMessage = AnsiToWstring(error.what());
            }
            catch (...)
            {
                lastErrorMessage = L"Unknown Mayohoshi renderer command error.";
            }
        }

        void recordOrApplyResource(RecordedCommand command)
        {
            try
            {
                if (initialized)
                {
                    RenderRuntime& app = requireRenderer();
                    app.RunResourceCommand([&]() { command(app); });
                }
                else
                {
                    recordedCommands.emplace_back(std::move(command));
                }
                lastErrorMessage.clear();
            }
            catch (const D3D12Exception& error)
            {
                lastErrorMessage = error.ErrorMessageString();
            }
            catch (const std::exception& error)
            {
                lastErrorMessage = AnsiToWstring(error.what());
            }
            catch (...)
            {
                lastErrorMessage = L"Unknown Mayohoshi renderer resource command error.";
            }
        }

        RenderRuntime& requireRenderer()
        {
            if (!renderer)
                renderer = std::make_unique<RenderRuntime>(instance);
            return *renderer;
        }

    private:
        HINSTANCE instance = nullptr;
        std::unique_ptr<RenderRuntime> renderer;
        std::vector<RecordedCommand> recordedCommands;
        std::wstring lastErrorMessage;
        uint32_t directionalLightCount = 0;
        uint32_t pointLightCount = 0;
        uint32_t spotLightCount = 0;
        bool initialized = false;
    };

    MayohoshiRender::MayohoshiRender(HINSTANCE instance)
        : impl(std::make_unique<Impl>(instance))
    {
    }

    MayohoshiRender::~MayohoshiRender() = default;

    bool MayohoshiRender::Initialize(const RendererDesc& desc) { return impl->Initialize(desc); }
    bool MayohoshiRender::AttachToWindow(HWND hwnd, UINT width, UINT height) { return impl->AttachToWindow(hwnd, width, height); }
    bool MayohoshiRender::CreateRenderWindow(UINT width, UINT height, const std::wstring& title) { return impl->CreateRenderWindow(width, height, title); }
    int MayohoshiRender::RunMessageLoop() const { return impl->RunMessageLoop(); }
    void MayohoshiRender::Stop() { impl->Stop(); }
    const std::wstring& MayohoshiRender::GetLastErrorMessage() const { return impl->GetLastErrorMessage(); }
    void MayohoshiRender::Resize(UINT width, UINT height) { impl->Resize(width, height); }

    CanvasHandle MayohoshiRender::CreateCanvas(const CanvasDesc& desc) { return impl->CreateCanvas(desc); }
    CanvasHandle MayohoshiRender::CreateCanvas(const std::string& name, UINT width, UINT height) { return impl->CreateCanvas(name, width, height); }
    TextureHandle MayohoshiRender::CreateTexture(const TextureDesc& desc) { return impl->CreateTexture(desc); }
    TextureHandle MayohoshiRender::CreateTexture(const std::string& name, const std::wstring& filePath) { return impl->CreateTexture(name, filePath); }
    TextureSetHandle MayohoshiRender::BindTextures(const TextureSetDesc& desc) { return impl->BindTextures(desc); }
    TextureSetHandle MayohoshiRender::BindTextures(const std::string& name, const TextureHandle& diffuse, const TextureHandle& normal, const TextureHandle& depth) { return impl->BindTextures(name, diffuse, normal, depth); }
    MaterialHandle MayohoshiRender::CreateMaterial(const MaterialDesc& desc) { return impl->CreateMaterial(desc); }
    MaterialHandle MayohoshiRender::CreateMaterial(const std::string& name, const DirectX::XMFLOAT4& albedo, float roughness, float metallic, RenderLayer renderLayer) { return impl->CreateMaterial(name, albedo, roughness, metallic, renderLayer); }
    ModelHandle MayohoshiRender::CreateCylinder(const CylinderDesc& desc) { return impl->CreateCylinder(desc); }
    ModelHandle MayohoshiRender::CreateCylinder(const std::string& name, float bottomRadius, float topRadius, float height, uint32_t sliceCount, uint32_t stackCount, RenderLayer layer) { return impl->CreateCylinder(name, bottomRadius, topRadius, height, sliceCount, stackCount, layer); }
    ModelHandle MayohoshiRender::CreateSphere(const SphereDesc& desc) { return impl->CreateSphere(desc); }
    ModelHandle MayohoshiRender::CreateSphere(const std::string& name, float radius, uint32_t sliceCount, uint32_t stackCount, RenderLayer layer) { return impl->CreateSphere(name, radius, sliceCount, stackCount, layer); }
    ModelHandle MayohoshiRender::CreateGrid(const GridDesc& desc) { return impl->CreateGrid(desc); }
    ModelHandle MayohoshiRender::CreateGrid(const std::string& name, float width, float depth, uint32_t m, uint32_t n, RenderLayer layer) { return impl->CreateGrid(name, width, depth, m, n, layer); }
    ModelHandle MayohoshiRender::CreateModel(const ObjModelDesc& desc) { return impl->CreateModel(desc); }
    ModelHandle MayohoshiRender::CreateModel(const std::string& name, const std::wstring& filePath, RenderLayer layer, bool use32BitIndices) { return impl->CreateModel(name, filePath, layer, use32BitIndices); }
    RenderObjectHandle MayohoshiRender::AddInstance(const DrawDesc& desc) { return impl->AddInstance(desc); }
    RenderObjectHandle MayohoshiRender::AddInstance(const std::string& name, const ModelHandle& model, const MaterialHandle& material, const TextureSetHandle& textures, const TransformDesc& transform, TextureFlags textureFlags) { return impl->AddInstance(name, model, material, textures, transform, textureFlags); }
    RenderObjectHandle MayohoshiRender::AddInstances(const InstanceBatchDesc& desc) { return impl->AddInstances(desc); }
    RenderObjectHandle MayohoshiRender::AddInstances(const std::string& name, const ModelHandle& model, const std::vector<RenderInstanceDesc>& instances) { return impl->AddInstances(name, model, instances); }
    RenderObjectHandle MayohoshiRender::AddToRender(const DrawDesc& desc) { return impl->AddToRender(desc); }
    void MayohoshiRender::DrawTo(const CanvasHandle& canvas) { impl->DrawTo(canvas); }
    void MayohoshiRender::DrawToScreen(const CanvasHandle& canvas) { impl->DrawToScreen(canvas); }

    void MayohoshiRender::SetAmbientLight(const DirectX::XMFLOAT3& color, float intensity) { impl->SetAmbientLight(color, intensity); }
    LightHandle MayohoshiRender::AddLight(const LightDesc& desc) { return impl->AddLight(desc); }
    LightHandle MayohoshiRender::AddDirectionalLight(const DirectX::XMFLOAT3& direction, const DirectX::XMFLOAT3& intensity, const std::string& name) { return impl->AddDirectionalLight(direction, intensity, name); }
    LightHandle MayohoshiRender::AddPointLight(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& intensity, float range, const std::string& name) { return impl->AddPointLight(position, intensity, range, name); }
    LightHandle MayohoshiRender::AddSpotLight(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& direction, const DirectX::XMFLOAT3& intensity, float range, float spotPower, const std::string& name) { return impl->AddSpotLight(position, direction, intensity, range, spotPower, name); }
    bool MayohoshiRender::SetLight(const LightHandle& light, const LightDesc& desc) { return impl->SetLight(light, desc); }
    bool MayohoshiRender::SetLightPosition(const LightHandle& light, const DirectX::XMFLOAT3& position) { return impl->SetLightPosition(light, position); }
    bool MayohoshiRender::SetLightDirection(const LightHandle& light, const DirectX::XMFLOAT3& direction) { return impl->SetLightDirection(light, direction); }
    bool MayohoshiRender::SetLightIntensity(const LightHandle& light, const DirectX::XMFLOAT3& intensity) { return impl->SetLightIntensity(light, intensity); }
    void MayohoshiRender::ClearLights() { impl->ClearLights(); }
    void MayohoshiRender::SetUpdateCallback(UpdateCallback callback) { impl->SetUpdateCallback(std::move(callback)); }

    bool MayohoshiRender::SetObjectTransform(const RenderObjectHandle& object, const TransformDesc& transform, size_t instanceIndex) { return impl->SetObjectTransform(object, transform, instanceIndex); }
    void MayohoshiRender::SetCameraPosition(const DirectX::XMFLOAT3& position) { impl->SetCameraPosition(position); }
    void MayohoshiRender::LookAt(const DirectX::XMFLOAT3& target) { impl->LookAt(target); }
    void MayohoshiRender::SetWireframe(bool enabled) { impl->SetWireframe(enabled); }
}
