# Mayohoshi Render

According to the Dragon-Book(3D Game Programming With DirectX 12),the project has implemented the Ch.10.

## Usage

Build `Mayohoshi-Render.sln` with `x64` and run the `Mayohoshi-Render` demo project. The demo expects its working directory to be `Mayohoshi-Render/Mayohoshi-Render`, so Visual Studio debugging is configured to use `$(ProjectDir)`.

To embed the renderer in another Win32 application:

- Add `Mayohoshi-Render/Public` to the include directories.
- Link against `Mayohoshi-Render/lib/x64/Debug/MayohoshiRender.lib` or the matching configuration output.
- Include `Mayohoshi.h`.
- Create `Mayohoshi::Renderer`, register resources and draw commands, then call `AttachToWindow` or `Initialize` with your own `HWND`.
- Set `RendererDesc::shaderDirectory` to the shader directory visible from your application working directory. Asset paths passed to `CreateTexture` and `CreateModel` should also be paths your application can resolve.

Example:

```cpp
#include "Mayohoshi.h"

Mayohoshi::Renderer renderer(hInstance);

Mayohoshi::RendererDesc desc;
desc.instance = hInstance;
desc.targetWindow = hwnd;
desc.createOwnWindow = false;
desc.width = width;
desc.height = height;
desc.shaderDirectory = L"D:/Mayohoshi-Render/Shaders/";

auto screen = renderer.CreateCanvas({ "Screen" });
auto texture = renderer.CreateTexture({ "brick", L"D:/Mayohoshi-Render/Resources/Textures/bricks.dds" });
// Create material/model, bind textures, AddInstance or AddInstances, DrawTo, DrawToScreen...

renderer.Initialize(desc);
```

## Have Done：

- Ch.4 D3D12_Init
- Ch.5 Pipeline_Build
- Ch.6 DrawCall(Static)
- Ch.7 DrawCall(Dynamic)
- Ch.8 Lighting(Material) PBR(Based-on-Disney-BRDF)
- Ch.9 Texture
- Ch.10 Alpha_Blending
- Ch.15 Class_Camera Dynamic_Index
- Ch.19 Normal_Mapping
- Ch.18 Skycube
- Ch.19 Displacement_Mapping

## TODO:
- Ch.18 Dynamic_Texture
- Ch.22 Quaternion
- Ch.21 AO/SSAO
- Ch.20 Shadowmapping
- Ch.11 Reflect(Stencil)
- Ch.13 Compute_Shader
- Ch.18 Skycube(Environment_Illusion)
