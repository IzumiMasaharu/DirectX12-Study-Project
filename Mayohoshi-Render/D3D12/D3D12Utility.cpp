#include "D3D12Utility.h"

using namespace DirectX;
using namespace Microsoft::WRL;

D3D12Exception::D3D12Exception(HRESULT hr, const std::wstring& function_name, const std::wstring& file_name, UINT line_num)
	:errorCode(hr),functionName(function_name),fileName(file_name),lineNum(line_num)
{
	OutputDebugString(ErrorMessageString().c_str());
}
// 读取错误信息，并将错误信息转化为可输出的字符串
std::wstring D3D12Exception::ErrorMessageString()const
{
	_com_error err(errorCode);
	std::wstring msg = err.ErrorMessage();

	return L"\n" + functionName + L"\n错误位于：" + fileName + L"第" + std::to_wstring(lineNum) + L"行;\n错误内容: " + msg + L"\n\n";
}

// 创建默认缓冲区
ComPtr<ID3D12Resource> D3D12Utility::CreateDefaultBuffer(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    const void* initData,
    UINT64 byteSize,
    ComPtr<ID3D12Resource>& uploadBuffer)
{
    ComPtr<ID3D12Resource> defaultBuffer;

    // 创建默认缓冲区和上传缓冲区
    ThrowIfFailed(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
        D3D12_RESOURCE_STATE_COMMON,
        nullptr, IID_PPV_ARGS(defaultBuffer.GetAddressOf())));
    ThrowIfFailed(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,IID_PPV_ARGS(uploadBuffer.GetAddressOf())));

	// 描述上传到DefaultBuffer的数据
    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = initData;
    subResourceData.RowPitch = byteSize;
    subResourceData.SlicePitch = subResourceData.RowPitch;

    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_COPY_DEST));

	// 将上传缓冲区中的资源复制到GPU的默认缓冲区中
	// uploadBuffer ---Command：CopyTextureRegion()/CopyBufferRegion()（由UpdateSubresources调用cmdList完成）---> DefaultBuffer
	UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);

    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_GENERIC_READ));

	return defaultBuffer;
}

// 将数据大小字节对齐为256b以适配常量缓冲区
UINT D3D12Utility::ConstUploadBufferByteSize256Alignment(UINT ByteSize)
{
	return (ByteSize + 255) & ~255;
}

// 在线编译Shader
ComPtr<ID3DBlob> D3D12Utility::CompileShaderOnline(
	const std::wstring& hlsl_filename,
	const D3D_SHADER_MACRO* defines,
	const std::string& Entrypoint,
	const std::string& TargetShaderType)
{
	UINT compileFlags = D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES;
#if defined(DEBUG) || defined(_DEBUG)  
	compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
	auto hr = S_OK;
	ComPtr<ID3DBlob> byteCode = nullptr;
	ComPtr<ID3DBlob> errors;
	hr = D3DCompileFromFile(hlsl_filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		Entrypoint.c_str(), TargetShaderType.c_str(), compileFlags, 0, &byteCode, &errors);

	if (errors != nullptr)
		OutputDebugStringA((char*)errors->GetBufferPointer());

	if (FAILED(hr))
	{
		std::wstring message = L"D3DCompileFromFile failed: " + hlsl_filename;
		message += L"\nEntry point: " + AnsiToWstring(Entrypoint);
		message += L"\nTarget: " + AnsiToWstring(TargetShaderType);

		if (errors != nullptr)
		{
			const auto* errorText = static_cast<const char*>(errors->GetBufferPointer());
			message += L"\nCompiler output:\n";
			message += AnsiToWstring(std::string(errorText, errors->GetBufferSize()));
		}

		throw D3D12Exception(hr, message, AnsiToWstring(__FILE__), __LINE__);
	}

	return byteCode;
}

// 将二进制文件读作ID3DBlob文件（可用于载入离线编译的Shader.cso）
ComPtr<ID3DBlob> D3D12Utility::LoadBinaryToBlob(const std::wstring& Binary_filename)
{
	std::ifstream fin(Binary_filename, std::ios::binary);

	fin.seekg(0, std::ios_base::end);
	std::ifstream::pos_type size = (int)fin.tellg();
	fin.seekg(0, std::ios_base::beg);

	ComPtr<ID3DBlob> blob;
	ThrowIfFailed(D3DCreateBlob(size, blob.GetAddressOf()));

	fin.read((char*)blob->GetBufferPointer(), size);
	fin.close();

	return blob;
}

// 获取静态采样器
std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> D3D12Utility::GetStaticSamplers()
{
	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	return { pointWrap, pointClamp, linearWrap, linearClamp, anisotropicWrap, anisotropicClamp };
}
