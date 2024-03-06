#pragma once
#include <cstdint>
#include <DirectXMath.h>
#include <vector>

class GeometryGenerator
{
public:
	struct Vertex;
	struct MeshData;
public:
	//����Բ̨
	void CreateCylinderTop(float topRadius, float height, uint32_t sliceCount, MeshData& meshData)const;
	void CreateCylinderBottom(float bottomRadius, float height, uint32_t sliceCount, MeshData& meshData)const;
	MeshData CreateCylinder(float bottomRadius, float topRadius, float height, uint32_t sliceCount, uint32_t stackCount)const;
	//��������
	MeshData CreateBall(float Radius, uint32_t sliceCount, uint32_t stackCount)const;
	//��������ƽ��
	MeshData CreateGird(float length, float width, uint32_t xPointNum, uint32_t zPointNum)const;
};
//�洢�������������ĵ����������ݵĽṹ��
struct GeometryGenerator::Vertex
{
public:
	Vertex() = default;
	Vertex(const DirectX::XMFLOAT3& p, const DirectX::XMFLOAT3& n, const DirectX::XMFLOAT3& t, const DirectX::XMFLOAT2& uv) :Position(p), Normal(n), Tangent(t), Texture(uv) {}
	Vertex(
		float px, float py, float pz,
		float nx, float ny, float nz,
		float tx, float ty, float tz,
		float u, float v) :Position(px, py, pz), Normal(nx, ny, nz), Tangent(tx, ty, tz), Texture(u, v) {}
public:
	DirectX::XMFLOAT3 Position;//���涥���λ������
	DirectX::XMFLOAT3 Normal;//���漸���������ڶ��㴦����ĵ�λ������
	DirectX::XMFLOAT3 Tangent;//���漸���������ڶ��㴦������ƽ�еĵ�λ����
	DirectX::XMFLOAT2 Texture;//����ӳ�䵽��������ʱ�������ж�Ӧ����������
};
//�洢������������������������Ķ��㼯���������Ľṹ��
struct GeometryGenerator::MeshData
{
public:
	std::vector<uint16_t>& GetIndices_16();//��ȡ����unit16_t���͵�����vector
public:
	std::vector<GeometryGenerator::Vertex> Vertices;
	std::vector<uint32_t> Indices_32;
private:
	std::vector<uint16_t> mIndices_16;
};