#pragma once

#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"D3D12.lib")
#pragma comment(lib,"dxgi.lib")

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cmath>
#include <comdef.h>
#include <condition_variable>
#include <cstdint>
#include <DirectXCollision.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <d3d12.h>
#include <float.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <windowsx.h>
#include <Windows.h>
#include <wrl.h>
#include "d3dx12.h"
#include "d3d12.h"

// 为 DirectX 对象设置调试名称，以便在调试时更容易识别和跟踪这些对象
inline void d3dSetDebugName(IDXGIObject* obj, const char* name)
{
    if (obj)
        obj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(name), name);
}
inline void d3dSetDebugName(ID3D12Device* obj, const char* name)
{
    if (obj)
        obj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(name), name);
}
inline void d3dSetDebugName(ID3D12DeviceChild* obj, const char* name)
{
    if (obj)
        obj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(name), name);
}
// 将string转为wstring
inline std::wstring AnsiToWstring(const std::string& str)
{
    if (str.empty())
        return {};

    // 第一次调用，获取需要的 wchar_t 数量（包括终止符）
    int sizeNeeded = MultiByteToWideChar(CP_ACP, 0, str.c_str(), (int)str.size(),
        nullptr, 0);

    std::wstring result(sizeNeeded, L'\0');

    // 第二次调用，真正转换
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), (int)str.size(),
        &result[0], sizeNeeded);

    return result;
}
// 将wstring转为string
inline std::string WstringToAnsi(const std::wstring& str)
{
    if (str.empty())
        return {};

    int sizeNeeded = WideCharToMultiByte(CP_ACP, 0, str.c_str(),
        (int)str.size(),
        nullptr, 0, nullptr, nullptr);

    std::string result(sizeNeeded, '\0');

    WideCharToMultiByte(CP_ACP, 0, str.c_str(),
        (int)str.size(),
        &result[0], sizeNeeded, nullptr, nullptr);

    return result;
}


class DxException
{
public:
    DxException() = default;
    DxException(HRESULT hr, const std::wstring& function_name, const std::wstring& file_name, UINT line_num);

    std::wstring ErrorMessageString()const; // 读取错误信息，并将错误信息转化为可输出的字符串
public:
    HRESULT errorCode = S_OK;
    std::wstring functionName;
    std::wstring fileName;
    UINT lineNum = -1;
};

class DxUtil
{
public:
    // 疑问：为何要把UploadBuffer作为参数传递进函数 而不是在函数中创建一个ComPtr<ID3D12Resource> UploadBuffer完成资源传递操作?
    // 答：UploadBuffer不能立即销毁，因为命令列表复制操作在CreateDefaultBuffer（）调用完毕时可能尚未执行，必须等调用者得知复制完成后才能释放UploadBuffer。
    static Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        const void* initData,
        UINT64 byteSize,
        Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);

    // 将数据大小字节对齐为256b以适配常量缓冲区
    static UINT ConstUploadBufferByteSize256Alignment(UINT ByteSize); 

    // 在线编译着色器
    static Microsoft::WRL::ComPtr<ID3DBlob> CompileShaderOnline(
        const std::wstring& hlsl_filename,
        const D3D_SHADER_MACRO* defines,
        const std::string& Entrypoint,
        const std::string& TargetShaderType);

    // 将二进制字符串写进Blob文件
    static Microsoft::WRL::ComPtr<ID3DBlob> LoadBinaryToBlob(const std::wstring& Binary_filename);

    // 获取静态采样器
    static std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

};

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                              \
{                                                                     \
    HRESULT hr__ = (x);                                               \
    std::wstring wfn = AnsiToWstring(__FILE__);                       \
    if(FAILED(hr__)) { throw DxException(hr__, L#x, wfn, __LINE__); } \
}
#endif
// 释放动态指针
#ifndef ReleaseCom
#define ReleaseCom(x) { if(x){ x->Release(); x = 0; } }
#endif
