#pragma once

#include "MayohoshiRender.h"

namespace Mayohoshi
{
    using Renderer = MayohoshiRender;
    using RenderApplication = MayohoshiRender;
    using RenderRuntimeApplication = MayohoshiRender;

    inline TransformDesc MakeTransform(
        DirectX::CXMMATRIX world,
        DirectX::CXMMATRIX textureTransform)
    {
        TransformDesc desc;
        DirectX::XMStoreFloat4x4(&desc.world, world);
        DirectX::XMStoreFloat4x4(&desc.textureTransform, textureTransform);
        return desc;
    }

    inline TransformDesc MakeTransform(DirectX::CXMMATRIX world)
    {
        return MakeTransform(world, DirectX::XMMatrixIdentity());
    }
}
