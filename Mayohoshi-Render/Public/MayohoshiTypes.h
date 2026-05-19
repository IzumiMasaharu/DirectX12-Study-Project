#pragma once

#include <DirectXMath.h>
#include <Windows.h>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Mayohoshi
{
    using RenderLayer = uint32_t;
    using TextureFlags = uint32_t;
    using UpdateCallback = std::function<void(float totalTime, float deltaTime)>;

    constexpr float Pi = 3.14159265358979323846f;

    inline DirectX::XMFLOAT4X4 IdentityMatrix()
    {
        return {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };
    }

    namespace RenderLayers
    {
        constexpr RenderLayer Opaque = 0;
        constexpr RenderLayer Skybox = 100;
        constexpr RenderLayer Transparent = 200;
        constexpr RenderLayer Overlay = 300;
    }

    enum class TextureType : TextureFlags
    {
        TEX_NONE = 0u,
        TEX_DIFFUSE = 1u << 0,
        TEX_NORMAL = 1u << 1,
        TEX_DEPTH = 1u << 2,
    };

    inline constexpr TextureType operator|(TextureType a, TextureType b)
    {
        return static_cast<TextureType>(static_cast<TextureFlags>(a) | static_cast<TextureFlags>(b));
    }

    inline constexpr TextureType operator&(TextureType a, TextureType b)
    {
        return static_cast<TextureType>(static_cast<TextureFlags>(a) & static_cast<TextureFlags>(b));
    }

    inline constexpr TextureFlags ToTextureFlags(TextureType type)
    {
        return static_cast<TextureFlags>(type);
    }

    enum class CanvasFormat
    {
        Rgba8Unorm,
    };

    struct RendererDesc
    {
        HINSTANCE instance = nullptr;
        HWND targetWindow = nullptr;
        UINT width = 1280;
        UINT height = 720;
        std::wstring title = L"Mayohoshi Render";
        std::wstring shaderDirectory = L"../Shaders/";
        bool createOwnWindow = true;
        bool startThreads = true;
    };

    struct CanvasHandle
    {
        std::string name;
    };

    struct TextureHandle
    {
        std::string name;
    };

    struct TextureSetHandle
    {
        std::string name;
    };

    struct MaterialHandle
    {
        std::string name;
    };

    struct ModelHandle
    {
        std::string name;
        std::string geometryName;
        std::string submeshName;
        RenderLayer layer = RenderLayers::Opaque;
    };

    struct RenderObjectHandle
    {
        std::string name;
    };

    enum class LightType
    {
        Directional,
        Point,
        Spot,
    };

    struct LightHandle
    {
        LightType type = LightType::Directional;
        uint32_t index = 0;
        std::string name;
    };

    struct CanvasDesc
    {
        std::string name = "Screen";
        UINT width = 0;
        UINT height = 0;
        CanvasFormat format = CanvasFormat::Rgba8Unorm;
        bool useDepth = true;
    };

    struct TextureDesc
    {
        std::string name;
        std::wstring filePath;
    };

    struct TextureSetDesc
    {
        std::string name;
        TextureHandle diffuse;
        TextureHandle normal;
        TextureHandle depth;
    };

    struct MaterialDesc
    {
        std::string name;
        DirectX::XMFLOAT4 albedo = { 1.0f,1.0f,1.0f,1.0f };
        float metallic = 0.0f;
        float roughness = 0.5f;
        float ior = 1.5f;
        DirectX::XMFLOAT3 emissive = { 0.0f,0.0f,0.0f };
        DirectX::XMFLOAT4X4 materialTransform = IdentityMatrix();
        RenderLayer renderLayer = RenderLayers::Opaque;
    };

    struct TransformDesc
    {
        DirectX::XMFLOAT4X4 world = IdentityMatrix();
        DirectX::XMFLOAT4X4 textureTransform = IdentityMatrix();

        static TransformDesc FromMatrix(DirectX::CXMMATRIX worldMatrix)
        {
            TransformDesc desc;
            DirectX::XMStoreFloat4x4(&desc.world, worldMatrix);
            return desc;
        }
    };

    struct CylinderDesc
    {
        std::string name;
        float bottomRadius = 1.0f;
        float topRadius = 1.0f;
        float height = 1.0f;
        uint32_t sliceCount = 32;
        uint32_t stackCount = 8;
        RenderLayer layer = RenderLayers::Opaque;
    };

    struct SphereDesc
    {
        std::string name;
        float radius = 1.0f;
        uint32_t sliceCount = 32;
        uint32_t stackCount = 16;
        RenderLayer layer = RenderLayers::Opaque;
    };

    struct GridDesc
    {
        std::string name;
        float width = 1.0f;
        float depth = 1.0f;
        uint32_t m = 2;
        uint32_t n = 2;
        RenderLayer layer = RenderLayers::Opaque;
    };

    struct ObjModelDesc
    {
        std::string name;
        std::wstring filePath;
        bool use32BitIndices = true;
        RenderLayer layer = RenderLayers::Opaque;
    };

    struct DrawDesc
    {
        std::string name;
        ModelHandle model;
        MaterialHandle material;
        TextureSetHandle textures;
        TransformDesc transform;
        TextureFlags textureFlags = ToTextureFlags(TextureType::TEX_DIFFUSE);
    };

    struct RenderInstanceDesc
    {
        TransformDesc transform;
        MaterialHandle material;
        TextureSetHandle textures;
        TextureFlags textureFlags = ToTextureFlags(TextureType::TEX_DIFFUSE);
    };

    struct InstanceBatchDesc
    {
        std::string name;
        ModelHandle model;
        std::vector<RenderInstanceDesc> instances;
    };

    struct LightDesc
    {
        std::string name;
        LightType type = LightType::Directional;
        DirectX::XMFLOAT3 intensity = { 1.0f, 1.0f, 1.0f };
        DirectX::XMFLOAT3 direction = { -0.57735f, -0.57735f, 0.57735f };
        DirectX::XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };
        float start = 0.0f;
        float end = 10.0f;
        float spotPower = 64.0f;
    };

    struct CameraDesc
    {
        DirectX::XMFLOAT3 position = { 0.0f,0.0f,-10.0f };
        float nearZ = 0.1f;
        float farZ = 1000.0f;
    };
}
