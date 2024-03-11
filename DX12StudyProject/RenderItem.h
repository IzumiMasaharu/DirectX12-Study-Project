#pragma once
#include "DXBase.h"

const UINT gNumFrameResources = 3;//��������֡��Դ����

struct RenderItem
{
public:
	RenderItem() = default;
public:
	DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();//��Ⱦ�������任����
	DirectX::XMFLOAT4X4 TexTrans = MathHelper::Identity4x4();

	UINT numDirtyFrames = gNumFrameResources;//��¼�м���֡��Դ�е���Ⱦ�����ݴ�����
	UINT ObjectConstBufferIndex = -1;//����Ⱦ��ĳ�����������������Ⱦ��Ļ������е�����

	Material* Mat = nullptr;//����Ⱦ����ʹ�õĲ���
	MeshGeometry* Geo = nullptr;//����Ⱦ����ʹ�õ������弯
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;//ָ����Ⱦ���ͼԪ���˸�ʽ
	//����������������Geo���󶨵����������
	UINT indexCount = 0;
	uint32_t indexStartLocation = 0;
	uint32_t vertexBaseLocation = 0;
};