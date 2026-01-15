#pragma once
#include "HandlerWndMsgProc.h"

#include <string>
#include <unordered_map>
#include <memory>
#include <cstdint>

struct WindowClassDesc
{
	LPCWSTR className = L"DefaultWindowClass";
	uint32_t style = CS_HREDRAW | CS_VREDRAW;
	HICON icon = nullptr;
};
struct WindowDesc
{
	LPCWSTR title = L"DefaultWindow";
	int left = CW_USEDEFAULT;
	int top = CW_USEDEFAULT;
	int clientWidth = 640;
	int clientHeight = 360;
	bool resizable = true;
	bool maximized = false;
};

class WindowClass
{
public:
	WindowClass() = default;
	explicit WindowClass(HINSTANCE hInstance);
	~WindowClass();

	const wchar_t* setWCName(LPCWSTR WCName);

	const wchar_t* getWCName()const;
	HINSTANCE getInstance()const;
private:
	HINSTANCE windowClassInstance;
	const wchar_t* windowClassName = nullptr;
};
class Window
{
public:
	Window() = delete;
	explicit Window(const WindowDesc& wndDesc, const WindowClass& windowClass, HandlerWndMsgProc* handler);
	~Window();

	const wchar_t* setWndName(LPCWSTR wndName);
	void setWndPos(int x, int y, int cw, int ch);

	RECT getWndPos() const;
	HWND getHwnd() const;
private:
	HWND hWnd = nullptr;

	LPCWSTR windowName = nullptr;
	int32_t windowX = 0;
	int32_t windowY = 0;
	int32_t windowWidth = 0;
	int32_t windowHeight = 0;
};

class WindowsManager
{
public:
	WindowsManager() = delete;
	explicit WindowsManager(HINSTANCE hInstance,HandlerWndMsgProc* handler);
	~WindowsManager() = default;

	static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	bool initWindowClass(WindowClassDesc& wndClassDesc);
	bool initWindow(std::wstring wndKey, const WindowDesc& wndDesc);

	const wchar_t* setWndName(const std::wstring& wndKey, LPCWSTR wndName);	
	void setWndPos(const std::wstring& wndKey, int x, int y, int wx, int wy);
	
	const wchar_t* getWCName()const;
	HINSTANCE getWCInstance() const;
	HWND getHwnd(const std::wstring& wndKey) const;					
private:
	HINSTANCE appInstance;
	HandlerWndMsgProc* handler = nullptr;
	
	WindowClass windowClass;
	std::unordered_map<std::wstring, std::unique_ptr<Window>> windows;
};
