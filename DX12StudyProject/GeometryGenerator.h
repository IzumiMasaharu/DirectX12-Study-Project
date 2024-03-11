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
	//生成圆台
	void CreateCylinderTop(float topRadius, float height, uint32_t sliceCount, MeshData& meshData)const;
	void CreateCylinderBottom(float bottomRadius, float height, uint32_t sliceCount, MeshData& meshData)const;
	MeshData CreateCylinder(float bottomRadius, float topRadius, float height, uint32_t sliceCount, uint32_t stackCount)const;
	//生成球体
	MeshData CreateBall(float Radius, uint32_t sliceCount, uint32_t stackCount)const;
	//生成网格平面
	MeshData CreateGird(float length, float width, uint32_t xPointNum, uint32_t zPointNum)const;
	//生成Skull模型
	MeshData CreateSkull();
};
//存储几何体生成器的单个顶点数据的结构体
struct GeometryGenerator::Vertex
{
public:
	Vertex() = default;
	Vertex(const DirectX::XMFLOAT3& p, const DirectX::XMFLOAT3& n, const DirectX::XMFLOAT3& t, const DirectX::XMFLOAT2& uv) :position(p), Normal(n), Tangent(t), Texture(uv) {}
	Vertex(
		float px, float py, float pz,
		float nx, float ny, float nz,
		float tx, float ty, float tz,
		float u, float v) :position(px, py, pz), Normal(nx, ny, nz), Tangent(tx, ty, tz), Texture(u, v) {}
public:
	DirectX::XMFLOAT3 position;//储存顶点的位置坐标
	DirectX::XMFLOAT3 Normal;//储存几何网格体在顶点处顶点的单位法向量
	DirectX::XMFLOAT3 Tangent;//储存几何网格体在顶点处与切线平行的单位向量
	DirectX::XMFLOAT2 Texture;//纹理映射到网格体上时在纹理中对应的纹理坐标
};
//存储几何体生成器所生成网格体的顶点集和索引集的结构体
struct GeometryGenerator::MeshData
{
public:
	std::vector<uint16_t>& GetIndices_16();//获取存有unit16_t类型的索引vector
public:
	std::vector<GeometryGenerator::Vertex> Vertices;
	std::vector<uint32_t> Indices_32;
private:
	std::vector<uint16_t> mIndices_16;
};