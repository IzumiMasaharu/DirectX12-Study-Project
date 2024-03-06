#pragma once
#include"DXBase.h"
#include"UploadBuffer.h"

//����ṹ��
struct VertexPos
{
	XMFLOAT3 Pos;
};
struct VertexColor
{
	XMFLOAT4 Color;
};
//������ͼͶӰ����
struct ObjectConstants
{
	XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4();
};
//��Ⱦ���̳�������
struct RenderingPassConstants
{
    XMFLOAT4X4 View = MathHelper::Identity4x4();//�������ͼ����
    XMFLOAT4X4 InvView = MathHelper::Identity4x4();//��ͼ����������
    XMFLOAT4X4 Proj = MathHelper::Identity4x4();//ͶӰ������ʾ��Ļ������
    XMFLOAT4X4 InvProj = MathHelper::Identity4x4();//ͶӰ����������
    XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();//��ͼͶӰ����
    XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();//��ͼͶӰ����������
    XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };//�����λ������
    float cbPerObjectPad1 = 0.0f;//
    XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };//��ȾĿ��Ĵ�С
    XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };//��ȾĿ���С�ĵ���
    float NearZ = 0.0f;//����ƽ��
    float FarZ = 0.0f;//Զ��ƽ��
    float TotalTime = 0.0f;//����������ʱ��
    float DeltaTime = 0.0f;//����tick֮���ʱ���
};

//֡��Դ
struct Framesource
{
public:
    Framesource(ID3D12Device* device, UINT PassCount, UINT ObjectCount);
    Framesource(const Framesource& rhs) = delete;
    Framesource& operator=(const Framesource& rhs) = delete;
    ~Framesource()=default;
};
