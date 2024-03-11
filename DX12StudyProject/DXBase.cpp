#include "DXBase.h"

using namespace DirectX;
using namespace Microsoft::WRL;

DxException::DxException(HRESULT hr, const std::wstring& function_name, const std::wstring& file_name, UINT line_num)
	:errorCode(hr),functionName(function_name),fileName(file_name),lineNum(line_num)
{
	OutputDebugString(ErrorMessageString().c_str());
}
//��ȡ������Ϣ������������Ϣת��Ϊ��������ַ���
std::wstring DxException::ErrorMessageString()const
{
	_com_error err(errorCode);
	std::wstring msg = err.ErrorMessage();

	return L"\n" + functionName + L"\n����λ�ڣ�" + fileName + L"��" + std::to_wstring(lineNum) + L"��;\n��������: " + msg + L"\n\n";
}

//����Ĭ�ϻ�����
ComPtr<ID3D12Resource> DXBase::CreateDefaultBuffer(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    const void* initData,
    UINT64 byteSize,
    ComPtr<ID3D12Resource>& uploadBuffer)
{
    ComPtr<ID3D12Resource> defaultBuffer;

    //����Ĭ�ϻ��������ϴ�������
    ThrowIfFailed(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
        D3D12_RESOURCE_STATE_COMMON,
        nullptr, IID_PPV_ARGS(defaultBuffer.GetAddressOf())))
    ThrowIfFailed(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,IID_PPV_ARGS(uploadBuffer.GetAddressOf())))

    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = initData;
    subResourceData.RowPitch = byteSize;
    subResourceData.SlicePitch = subResourceData.RowPitch;

    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_COPY_DEST));
	//��CPU�ڴ��е���Դ���Ƶ�GPU��Ĭ�ϻ�������
	//subResourceData ---> uploadBuffer ---CopyTextureRegion/CopyBufferRegion---> DefaultBuffer
	UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);

    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_GENERIC_READ));

	return defaultBuffer;
}

//�����ݴ�С�ֽڶ���Ϊ256b�����䳣��������
UINT DXBase::ConstUploadBufferByteSize256Alignment(UINT ByteSize)
{
	return (ByteSize + 255) & ~255;
}

//���߱���Shader
ComPtr<ID3DBlob> DXBase::CompileShaderOnline(
	const std::wstring& hlsl_filename,
	const D3D_SHADER_MACRO* defines,
	const std::string& Entrypoint,
	const std::string& TargetShaderType)
{
	UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
	auto hr = S_OK;
	ComPtr<ID3DBlob> byteCode = nullptr;
	ComPtr<ID3DBlob> errors;
	hr = D3DCompileFromFile(hlsl_filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		Entrypoint.c_str(), TargetShaderType.c_str(), compileFlags, 0, &byteCode, &errors);

	if (errors != nullptr)
		OutputDebugStringA((char*)errors->GetBufferPointer());

	ThrowIfFailed(hr)

	return byteCode;
}

//���������ļ�����ID3DBlob�ļ�
ComPtr<ID3DBlob> DXBase::LoadBinaryToBlob(const std::wstring& Binary_filename)
{
	std::ifstream fin(Binary_filename, std::ios::binary);

	fin.seekg(0, std::ios_base::end);
	std::ifstream::pos_type size = (int)fin.tellg();
	fin.seekg(0, std::ios_base::beg);

	ComPtr<ID3DBlob> blob;
	ThrowIfFailed(D3DCreateBlob(size, blob.GetAddressOf()))

	fin.read((char*)blob->GetBufferPointer(), size);
	fin.close();

	return blob;
}

const float MathHelper::Infinity = FLT_MAX;
const float MathHelper::Pi = 3.1415926535f;
//��������ת��Ϊֱ������
XMVECTOR MathHelper::SphericalToCartesian(float radius, float theta, float phi)
{
	return XMVectorSet(
		radius * sinf(phi) * cosf(theta),
		radius * cosf(phi),
		radius * sinf(phi) * sinf(theta),
		1.0f);
}
//����M��������ת�þ���
XMMATRIX MathHelper::InverseTranspose(CXMMATRIX M)
{
	XMMATRIX A = M;
	A.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

	XMVECTOR det = XMMatrixDeterminant(A);//���أ�det A��det A��det A��det A����det A = |A|
	return XMMatrixTranspose(XMMatrixInverse(&det, A));
}
//��ʼ��4x4����Ϊ��λ����
XMFLOAT4X4 MathHelper::Identity4x4()
{
	static XMFLOAT4X4 I(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);

	return I;
}
// ����ֱ�������£�x��y���ڼ������µļ���
float MathHelper::AngleFromXY(float x, float y)
{
	float theta = 0.0f;

	if (x >= 0.0f)
	{
		theta = atanf(y / x);

		if (theta < 0.0f)
			theta += 2.0f * Pi;
	}
	else
		theta = atanf(y / x) + Pi;

	return theta;
}
XMVECTOR MathHelper::RandUnitVec3()
{
	XMVECTOR One = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
	XMVECTOR Zero = XMVectorZero();

	while (true)
	{
		XMVECTOR v = XMVectorSet(MathHelper::RandF(-1.0f, 1.0f), MathHelper::RandF(-1.0f, 1.0f), MathHelper::RandF(-1.0f, 1.0f), 0.0f);

		if (XMVector3Greater(XMVector3LengthSq(v), One))
			continue;

		return XMVector3Normalize(v);
	}
}
XMVECTOR MathHelper::RandHemisphereUnitVec3(XMVECTOR n)
{
	XMVECTOR One = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
	XMVECTOR Zero = XMVectorZero();

	while (true)
	{
		XMVECTOR v = XMVectorSet(MathHelper::RandF(-1.0f, 1.0f), MathHelper::RandF(-1.0f, 1.0f), MathHelper::RandF(-1.0f, 1.0f), 0.0f);

		if (XMVector3Greater(XMVector3LengthSq(v), One))
			continue;

		if (XMVector3Less(XMVector3Dot(n, v), Zero))
			continue;

		return XMVector3Normalize(v);
	}
}