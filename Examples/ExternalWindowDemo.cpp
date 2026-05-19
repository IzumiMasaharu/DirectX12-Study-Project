#include "Mayohoshi.h"
#include "DemoScene.h"

#include <Windows.h>

#include <memory>

namespace
{
    std::unique_ptr<Mayohoshi::Renderer> gRenderer;

    LRESULT CALLBACK HostWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
        case WM_CREATE:
        {
            const auto* create = reinterpret_cast<LPCREATESTRUCT>(lParam);
            gRenderer = std::make_unique<Mayohoshi::Renderer>(create->hInstance);

            MayohoshiExamples::BuildDemoScene(*gRenderer);

            RECT rect = {};
            GetClientRect(hwnd, &rect);

            Mayohoshi::RendererDesc desc;
            desc.instance = create->hInstance;
            desc.targetWindow = hwnd;
            desc.width = static_cast<UINT>(rect.right - rect.left);
            desc.height = static_cast<UINT>(rect.bottom - rect.top);
            desc.createOwnWindow = false;
            desc.shaderDirectory = L"../Shaders/";

            if (!gRenderer->Initialize(desc))
            {
                MessageBox(hwnd, gRenderer->GetLastErrorMessage().c_str(), L"Mayohoshi Render", MB_OK | MB_ICONERROR);
                return -1;
            }
            return 0;
        }
        case WM_SIZE:
            if (gRenderer && wParam != SIZE_MINIMIZED)
                gRenderer->Resize(LOWORD(lParam), HIWORD(lParam));
            return 0;
        case WM_DESTROY:
            if (gRenderer)
                gRenderer->Stop();
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
        }
    }
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int showCommand)
{
    const wchar_t className[] = L"MayohoshiExternalHost";

    WNDCLASS wc = {};
    wc.lpfnWndProc = HostWndProc;
    wc.hInstance = instance;
    wc.lpszClassName = className;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        className,
        L"External App Using MayohoshiRender",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1280,
        720,
        nullptr,
        nullptr,
        instance,
        nullptr);

    if (!hwnd)
        return -1;

    ShowWindow(hwnd, showCommand);

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return static_cast<int>(msg.wParam);
}
