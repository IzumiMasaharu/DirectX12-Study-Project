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

enum class TextureType : uint32_t
{
	TEX_NONE = 0u,
	TEX_DIFFUSE = 1u << 0,
	TEX_NORMAL = 1u << 1,
	TEX_DEPTH = 1u << 2,
};

inline TextureType operator|(TextureType a, TextureType b)
{
	return static_cast<TextureType>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline TextureType operator&(TextureType a, TextureType b)
{
	return static_cast<TextureType>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline bool HasFlag(TextureType value, TextureType flag)
{
	return (static_cast<uint32_t>(value) & static_cast<uint32_t>(flag)) != 0;
}