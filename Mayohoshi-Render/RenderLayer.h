#pragma once
#include <cstdint>
// 渲染层定义 - 用户可扩展
using RenderLayer = uint32_t;

namespace RenderLayers
{
    constexpr RenderLayer Opaque = 0;
    constexpr RenderLayer Skybox = 100;
    constexpr RenderLayer Transparent = 200;
    constexpr RenderLayer Overlay = 300;
    // 用户可以在这些值之间插入自定义层
}