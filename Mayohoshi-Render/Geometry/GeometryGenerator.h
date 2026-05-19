#pragma once
#include <cstdint>
#include <DirectXMath.h>
#include <vector>
#include <string>

class GeometryGenerator
{
public:
	struct Vertex;
	struct MeshData;
public:
	// 生成圆台
	static void CreateCylinderTop(float topRadius, float height, uint32_t sliceCount, MeshData& meshData);
	static void CreateCylinderBottom(float bottomRadius, float height, uint32_t sliceCount, MeshData& meshData);
	static MeshData CreateCylinder(float bottomRadius, float topRadius, float height, uint32_t sliceCount, uint32_t stackCount);
	// 生成球体
	static MeshData CreateBall(float Radius, uint32_t sliceCount, uint32_t stackCount);
	// 生成网格平面
	static MeshData CreateGird(float length, float width, uint32_t xPointNum, uint32_t zPointNum);
	// 创建通过文件导入的模型
	static MeshData CreateImportedGeometryFromOBJ(const std::wstring& filePath);

	static void ComputeTangents(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
};

// 存储几何体生成器的单个顶点数据的结构体
struct GeometryGenerator::Vertex
{
public:
	Vertex() = default;
	Vertex(const DirectX::XMFLOAT3& p, const DirectX::XMFLOAT3& n, const DirectX::XMFLOAT3& t, const DirectX::XMFLOAT2& uv) :position(p), normal(n), tangent(t), textureUV(uv) {}
	Vertex(
		float px, float py, float pz,
		float nx, float ny, float nz,
		float tx, float ty, float tz,
		float u, float v) :position(px, py, pz), normal(nx, ny, nz), tangent(tx, ty, tz), textureUV(u, v) {}
public:
	DirectX::XMFLOAT3 position; // 储存顶点的位置坐标
	DirectX::XMFLOAT3 normal; // 储存几何网格体在顶点处顶点的单位法向量
	DirectX::XMFLOAT3 tangent; // 储存几何网格体在顶点处与切线平行的单位向量
	DirectX::XMFLOAT2 textureUV; // 纹理映射到网格体上时在纹理中对应的纹理坐标
};
// 存储几何体生成器所生成网格体的顶点集和索引集的结构体
struct GeometryGenerator::MeshData
{
public:
	std::vector<uint16_t>& getIndices16(); // 获取存有unit16_t类型的索引vector
	std::vector<uint32_t>& getIndices32(); // 获取存有unit16_t类型的索引vector
public:
	std::vector<GeometryGenerator::Vertex> vertices;
	std::vector<uint32_t> indices32;
private:
	std::vector<uint16_t> indices16;
};