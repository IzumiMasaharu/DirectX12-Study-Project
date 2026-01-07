#pragma once
#include <Windows.h>

class HandlerWndMsgProc
{
public:
    virtual ~HandlerWndMsgProc() = default;

    virtual LRESULT wndMsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) = 0;
};
