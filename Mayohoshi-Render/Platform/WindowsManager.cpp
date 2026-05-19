#include "WindowsManager.h"

WindowClass::WindowClass(HINSTANCE hInstance) :windowClassInstance(hInstance) {}
WindowClass::~WindowClass()
{
    UnregisterClass(windowClassName, getInstance());
}
const wchar_t* WindowClass::setWCName(LPCWSTR WCName)
{
    windowClassName = WCName;
    return windowClassName;
}
const wchar_t* WindowClass::getWCName()const
{
    return windowClassName;
}
HINSTANCE WindowClass::getInstance() const
{
    return windowClassInstance;
}

Window::Window(const WindowDesc& wndDesc, const WindowClass& windowClass, HandlerWndMsgProc* handler)
{
    windowName = wndDesc.title;

    // 1. client rect
    RECT rc{0, 0, wndDesc.clientWidth, wndDesc.clientHeight};

    // 2. client → window
    if (!AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, false))
        MessageBoxW(nullptr, L"AdjustWindowRect Failed.", nullptr, 0);

    // 3. 记录“请求的 window 尺寸”
    windowX = wndDesc.left;
    windowY = wndDesc.top;
    windowWidth = rc.right - rc.left;
    windowHeight = rc.bottom - rc.top;

    // 3. 创建窗口（x/y 是 window 左上角）
    hWnd = CreateWindowEx(
        0,
        windowClass.getWCName(),
        windowName,
        WS_OVERLAPPED | WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU | WS_THICKFRAME,
        windowX,
        windowY,
        windowWidth,
        windowHeight,
        nullptr,
        nullptr,
        windowClass.getInstance(),
        handler
    );

    if (!hWnd)
        MessageBoxW(nullptr, L"CreateWindow Failed.", nullptr, 0);

    // 在调用 ShowWindow 前增加空指针检查，防止 hWnd 为 nullptr
    if (hWnd != nullptr)
    {
        ShowWindow(hWnd, SW_SHOW);
        UpdateWindow(hWnd);
    }

    // 创建WM_INPUT设备
    RAWINPUTDEVICE rid{};
    rid.usUsagePage = 0x01; // Generic Desktop
    rid.usUsage     = 0x06; // Keyboard
    rid.dwFlags     = RIDEV_INPUTSINK; // 即使不在焦点也接收
    rid.hwndTarget  = hWnd;
    RegisterRawInputDevices(&rid, 1, sizeof(rid));
}
Window::~Window()
{
    if (hWnd)
        DestroyWindow(hWnd);
}
const wchar_t* Window::setWndName(LPCWSTR wndName)
{
    windowName = wndName;
    return windowName;
}
void Window::setWndPos(int x, int y, int cw, int ch)
{
    // 1. 构造理想 client rect
    RECT rc{0,0,cw,ch};

    // 2. client → window
    if (!AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, false))
        MessageBoxW(nullptr, L"AdjustWindowRect Failed.", nullptr, 0);

    // 3. 计算 window 尺寸
	windowX = x;
	windowY = y;
    windowWidth = rc.right - rc.left;
    windowHeight = rc.bottom - rc.top;
}
RECT Window::getWndPos() const 
{
    return RECT{ windowX, windowY, windowWidth, windowHeight };
}
HWND Window::getHwnd() const
{
    return hWnd;
}

WindowsManager::WindowsManager(HINSTANCE hInstance, HandlerWndMsgProc* handler) :appInstance(hInstance),windowClass(hInstance), handler(handler) { }
LRESULT CALLBACK WindowsManager::wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HandlerWndMsgProc* handler = nullptr;

    if (msg == WM_NCCREATE)
    {
        auto cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        handler = static_cast<HandlerWndMsgProc*>(cs->lpCreateParams);

        SetWindowLongPtr(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(handler));
        return TRUE;
    }
    else
    {
        handler = reinterpret_cast<HandlerWndMsgProc*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (handler)
        return handler->wndMsgProc(hwnd, msg, wParam, lParam);

    return DefWindowProc(hwnd, msg, wParam, lParam);
}
// 初始化窗口类
bool WindowsManager::initWindowClass(WindowClassDesc& wndClassDesc)
{
    windowClass.setWCName(wndClassDesc.className);

    WNDCLASSEX wndClass = { 0 };
    wndClass.cbSize = sizeof(wndClass);
    wndClass.hInstance = appInstance;
    wndClass.lpszClassName = windowClass.getWCName();
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
    wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wndClass.hIconSm = nullptr;
    wndClass.hIcon = wndClassDesc.icon;
    wndClass.hIconSm = nullptr;
    wndClass.lpfnWndProc = wndProc;
    wndClass.lpszMenuName = nullptr;
    wndClass.style = wndClassDesc.style;

    // 注册所填写窗口类
    if (!RegisterClassEx(&wndClass))
    {
        MessageBox(nullptr, L"RegisterWindowClass Failed.", nullptr, 0);
        return false;
    }

    return true;
}

// 初始化窗口
bool WindowsManager::initWindow(std::wstring wndKey, const WindowDesc& wndDesc)
{
    windows.emplace(wndKey, std::make_unique<Window>(wndDesc, windowClass, handler)); 
    
    return true;
}

const wchar_t* WindowsManager::setWndName(const std::wstring& wndKey, LPCWSTR wndName)
{
    return windows.at(wndKey)->setWndName(wndName);
}
void WindowsManager::setWndPos(const std::wstring& wndKey, int x, int y, int wx, int wy)
{
    windows.at(wndKey)->setWndPos(x, y, wx, wy);
}
const wchar_t* WindowsManager::getWCName()const
{
    return windowClass.getWCName();
}
HINSTANCE WindowsManager::getWCInstance() const
{
    return windowClass.getInstance();
}
HWND WindowsManager::getHwnd(const std::wstring& wndKey) const
{
    return windows.at(wndKey)->getHwnd();
}