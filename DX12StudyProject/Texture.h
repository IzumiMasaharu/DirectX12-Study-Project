#pragma once
#include <string>
#include <Windows.h>
#include <wrl.h>

// 存储纹理数据的结构体
struct Texture
{
    std::string name;
    UINT srvHeapIndex = -1; // 纹理在SRV堆中的索引

    Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> uploadHeap = nullptr;
};