#pragma once
#include "DXBase.h"
#include "UploadBuffer.h"

//����ɫ���󶨵Ķ���ṹ��
struct VertexInfo
{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT4 Color;
};

//����ɫ���󶨵Ĳ��ʽṹ��
struct MaterialConstants
{
    DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f,1.0f,1.0f,1.0f };//�����䷴����
    DirectX::XMFLOAT3 FresneRf0 = { 0.0f,0.0f,0.0f };//������ЧӦ��������Rf��0�㣩
    float Roughness = 0.0f;//���ʴֲڶ�

    DirectX::XMFLOAT4X4 MaterialTransform = MathHelper::Identity4x4();
};
//����ɫ���󶨵�������ͼͶӰ�������峣����������
struct ObjectConstants
{
    DirectX::XMFLOAT4X4 XMWorld = MathHelper::Identity4x4();//���������任����
};
//����ɫ���󶨵���Ⱦ���̳����ṹ��
struct RenderingPassConstants
{
    DirectX::XMFLOAT4X4 View = MathHelper::Identity4x4();//�������ͼ����
    DirectX::XMFLOAT4X4 InvView = MathHelper::Identity4x4();//��ͼ����������
    DirectX::XMFLOAT4X4 Proj = MathHelper::Identity4x4();//ͶӰ������ʾ��Ļ������
    DirectX::XMFLOAT4X4 InvProj = MathHelper::Identity4x4();//ͶӰ����������
    DirectX::XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();//��ͼͶӰ����
    DirectX::XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();//��ͼͶӰ����������
    DirectX::XMFLOAT3 EyePosW = { 0.0f,0.0f,0.0f };//�����λ������
    float cbPerObjectPad1 = 0.0f;//
    DirectX::XMFLOAT2 RenderTargetSize = { 1.0f,1.0f };//��ȾĿ��Ĵ�С
    DirectX::XMFLOAT2 InvRenderTargetSize = { 1.0f,1.0f };//��ȾĿ���С�ĵ���
    float NearZ = 0.0f;//����ƽ��
    float FarZ = 0.0f;//Զ��ƽ��
    float TotalTime = 0;//����������ʱ��
    float DeltaTime = 0;//����tick֮���ʱ���
};

//֡��Դ
struct FrameResource
{
public:
    FrameResource(ID3D12Device* device, UINT PassCount, UINT ObjectCount, UINT MaterialCount);
    FrameResource(const FrameResource& rhs) = delete;
    ~FrameResource() = default;
public:
    FrameResource& operator=(const FrameResource& rhs) = delete;
public:
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandAllocator;//ÿ��֡��Դ�����������
    std::unique_ptr<UploadBuffer<RenderingPassConstants>> PassConstBuffer = nullptr;//ÿ��֡��Դ����Ⱦ���̳���������
    std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectConstBuffer=nullptr;//ÿ��֡��Դ�����峣��������
    std::unique_ptr<UploadBuffer<MaterialConstants>> MaterialConstBuffer = nullptr;//ÿ��֡��Դ�Ĳ��ʳ���������
    UINT64 Fence = 0;
};

