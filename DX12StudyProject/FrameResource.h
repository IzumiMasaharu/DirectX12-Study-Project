#pragma once
#define MAX_NUM_LIGHTS 8
#include "DXBase.h"
#include "UploadBuffer.h"

//����ɫ���󶨵Ķ���ṹ��
struct VertexConstants
{
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT2 texture;
};

//����ɫ���󶨵Ĳ��ʽṹ��
struct MaterialConstants
{
    DirectX::XMFLOAT4 diffuseAlbedo = { 1.0f,1.0f,1.0f,1.0f };//�����䷴����
    DirectX::XMFLOAT3 fresneRf0 = { 0.0f,0.0f,0.0f };//������ЧӦ��������Rf��0�㣩
    float roughness = 0.0f;//���ʴֲڶ�
    DirectX::XMFLOAT4X4 materialTransform = MathHelper::Identity4x4();
};
//����ɫ���󶨵�������ͼͶӰ�������峣����������
struct ObjectConstants
{
    DirectX::XMFLOAT4X4 XMWorld = MathHelper::Identity4x4();//���������任����
};
//����ɫ���󶨵���Ⱦ���̳����ṹ��
struct RenderingPassConstants
{
    DirectX::XMFLOAT4X4 view = MathHelper::Identity4x4();//�������ͼ����
    DirectX::XMFLOAT4X4 invView = MathHelper::Identity4x4();//��ͼ����������
    DirectX::XMFLOAT4X4 proj = MathHelper::Identity4x4();//ͶӰ������ʾ��Ļ������
    DirectX::XMFLOAT4X4 invProj = MathHelper::Identity4x4();//ͶӰ����������
    DirectX::XMFLOAT4X4 viewProj = MathHelper::Identity4x4();//��ͼͶӰ����
    DirectX::XMFLOAT4X4 invViewProj = MathHelper::Identity4x4();//��ͼͶӰ����������
    DirectX::XMFLOAT3 eyePosW = { 0.0f,0.0f,0.0f };//�����λ������
    float cbPerObjectPad1 = 0.0f;//
    DirectX::XMFLOAT2 renderTargetSize = { 1.0f,1.0f };//��ȾĿ��Ĵ�С
    DirectX::XMFLOAT2 invRenderTargetSize = { 1.0f,1.0f };//��ȾĿ���С�ĵ���
    float nearZ = 0.0f;//����ƽ��
    float farZ = 0.0f;//Զ��ƽ��
    float totalTime = 0;//����������ʱ��
    float deltaTime = 0;//����tick֮���ʱ���
    DirectX::XMFLOAT4 ambientIlluminating= { 0.0f,0.0f,0.0f,1.0f };//����������

    Light lights[MAX_NUM_LIGHTS];
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
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;//ÿ��֡��Դ�����������
    std::unique_ptr<UploadBuffer<RenderingPassConstants>> passConstBuffer = nullptr;//ÿ��֡��Դ����Ⱦ���̳���������
    std::unique_ptr<UploadBuffer<ObjectConstants>> objectConstBuffer=nullptr;//ÿ��֡��Դ�����峣��������
    std::unique_ptr<UploadBuffer<MaterialConstants>> materialConstBuffer = nullptr;//ÿ��֡��Դ�Ĳ��ʳ���������
    UINT64 fence = 0;
};

