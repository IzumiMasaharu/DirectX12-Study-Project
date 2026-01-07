#include "GeometryGenerator.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <Windows.h>

using namespace DirectX;

// 生成圆台
void GeometryGenerator::CreateCylinderTop(float topRadius, float height, uint32_t sliceCount, MeshData& meshData)
{
	auto baseIndex = (uint32_t)meshData.vertices.size();

	float y = 0.5f * height;
	float dTheta = (2.0f * XM_PI) / sliceCount;

	for (uint32_t i = 0; i <= sliceCount; i++)
	{
		float x = topRadius * cosf(i * dTheta);
		float z = topRadius * sinf(i * dTheta);

		float u = x / height + 0.5f;
		float v = z / height + 0.5f;

		meshData.vertices.push_back(Vertex(x, y, z, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v));
	}

	meshData.vertices.push_back(Vertex(0.0f, y, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f));

	for (uint32_t i = 0; i < sliceCount; i++)
	{
		meshData.indices32.push_back((uint32_t)meshData.vertices.size() - 1);
		meshData.indices32.push_back(baseIndex + i + 1);
		meshData.indices32.push_back(baseIndex + i);
	}
}
void GeometryGenerator::CreateCylinderBottom(float bottomRadius, float height, uint32_t sliceCount, MeshData& meshData)
{
	auto baseIndex = (uint32_t)meshData.vertices.size();

	float y = -0.5f * height;
	float dTheta = (2.0f * XM_PI) / sliceCount;

	for (uint32_t i = 0; i <= sliceCount; i++)
	{
		float x = bottomRadius * cosf(i * dTheta);
		float z = bottomRadius * sinf(i * dTheta);

		float u = x / height + 0.5f;
		float v = z / height + 0.5f;

		meshData.vertices.push_back(Vertex(x, y, z, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v));
	}

	meshData.vertices.push_back(Vertex(0.0f, y, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f));

	for (uint32_t i = 0; i < sliceCount; i++)
	{
		meshData.indices32.push_back((uint32_t)meshData.vertices.size() - 1);
		meshData.indices32.push_back(baseIndex + i);
		meshData.indices32.push_back(baseIndex + i+1);
	}
}
GeometryGenerator::MeshData GeometryGenerator::CreateCylinder(float bottomRadius, float topRadius, float height, uint32_t sliceCount, uint32_t stackCount)
{
	MeshData MeshCylinderData;
	float stackHeight = height / stackCount;
	float deltaRadius = (topRadius - bottomRadius) / stackCount;
	uint32_t ringCount = stackCount + 1;
	float dTheta = 2.0f * XM_PI / sliceCount;

	for (uint32_t i = 0; i < ringCount; i++)
	{
		float y = -0.5f * height + stackHeight * i;
		float r = bottomRadius + deltaRadius * i;
		
		for (uint32_t j = 0; j <= sliceCount; j++)
		{
			Vertex vertex;
			float theta = j * dTheta;
			float x = r * cosf(theta);
			float z = r * sinf(theta);

			vertex.position = XMFLOAT3(x, y, z);
			vertex.textureUV.x = (float)j / sliceCount;
			vertex.textureUV.y = 1.0f-(float)i / stackCount;

			vertex.tangent = XMFLOAT3(-sinf(theta), 0.0f, cosf(theta));

			float dR = bottomRadius - topRadius;
			XMFLOAT3 bitangent(dR * cosf(theta), -height, dR * sinf(theta));
			XMVECTOR T = XMLoadFloat3(&vertex.tangent);
			XMVECTOR B = XMLoadFloat3(&bitangent);
			XMVECTOR N = XMVector3Normalize(XMVector3Cross(T, B));
			XMStoreFloat3(&vertex.normal, N);

			MeshCylinderData.vertices.push_back(vertex);
		}
	}

	uint32_t VertexPerRing = sliceCount + 1;
	for (uint32_t i = 0; i < stackCount; i++)
	{
		for (uint32_t j = 0; j < sliceCount; j++)
		{
			MeshCylinderData.indices32.push_back(i * VertexPerRing + j);
			MeshCylinderData.indices32.push_back((i + 1) * VertexPerRing + j);
			MeshCylinderData.indices32.push_back((i + 1) * VertexPerRing + j + 1);

			MeshCylinderData.indices32.push_back(i * VertexPerRing + j);
			MeshCylinderData.indices32.push_back((i + 1) * VertexPerRing + j + 1);
			MeshCylinderData.indices32.push_back(i * VertexPerRing + j + 1);
		}
	}

	CreateCylinderTop(topRadius, height, sliceCount, MeshCylinderData);
	CreateCylinderBottom(bottomRadius, height, sliceCount, MeshCylinderData);

	return MeshCylinderData;
}

// 生成球体
GeometryGenerator::MeshData GeometryGenerator::CreateBall(float Radius, uint32_t sliceCount, uint32_t stackCount)
{
	MeshData MeshBallData;

	Vertex topVertex(0.0f, +Radius, 0.0f, 0.0f, +1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	Vertex bottomVertex(0.0f, -Radius, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

	MeshBallData.vertices.push_back(topVertex);

	float phiStep = XM_PI / stackCount;
	float thetaStep = 2.0f * XM_PI / sliceCount;

	for (uint32_t i = 1; i <= stackCount - 1; ++i)
	{
		float phi = i * phiStep;

		for (uint32_t j = 0; j <= sliceCount; ++j)
		{
			float theta = j * thetaStep;

			Vertex v;

			// spherical to cartesian
			v.position.x = Radius * sinf(phi) * cosf(theta);
			v.position.y = Radius * cosf(phi);
			v.position.z = Radius * sinf(phi) * sinf(theta);

			// Partial derivative of P with respect to theta
			v.tangent.x = -Radius * sinf(phi) * sinf(theta);
			v.tangent.y = 0.0f;
			v.tangent.z = +Radius * sinf(phi) * cosf(theta);

			XMVECTOR T = XMLoadFloat3(&v.tangent);
			XMStoreFloat3(&v.tangent, XMVector3Normalize(T));

			XMVECTOR p = XMLoadFloat3(&v.position);
			XMStoreFloat3(&v.normal, XMVector3Normalize(p));

			v.textureUV.x = theta / XM_2PI;
			v.textureUV.y = phi / XM_PI;

			MeshBallData.vertices.push_back(v);
		}
	}

	MeshBallData.vertices.push_back(bottomVertex);

	for (uint32_t i = 1; i <= sliceCount; ++i)
	{
		MeshBallData.indices32.push_back(0);
		MeshBallData.indices32.push_back(i + 1);
		MeshBallData.indices32.push_back(i);
	}

	uint32_t baseIndex = 1;
	uint32_t ringVertexCount = sliceCount + 1;
	for (uint32_t i = 0; i < stackCount - 2; i++)
	{
		for (uint32_t j = 0; j < sliceCount; j++)
		{
			MeshBallData.indices32.push_back(baseIndex + i * ringVertexCount + j);
			MeshBallData.indices32.push_back(baseIndex + i * ringVertexCount + j + 1);
			MeshBallData.indices32.push_back(baseIndex + (i + 1) * ringVertexCount + j);
		
			MeshBallData.indices32.push_back(baseIndex + (i + 1) * ringVertexCount + j);
			MeshBallData.indices32.push_back(baseIndex + i * ringVertexCount + j + 1);
			MeshBallData.indices32.push_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
		}
	}

	uint32_t southPoleIndex = (uint32_t)MeshBallData.vertices.size() - 1;

	baseIndex = southPoleIndex - ringVertexCount;

	for (uint32_t i = 0; i < sliceCount; i++)
	{
		MeshBallData.indices32.push_back(southPoleIndex);
		MeshBallData.indices32.push_back(baseIndex + i);
		MeshBallData.indices32.push_back(baseIndex + i + 1);
	}

	return MeshBallData;
}

// 生成网格平面
GeometryGenerator::MeshData GeometryGenerator::CreateGird(float length, float width, uint32_t xPointNum, uint32_t zPointNum)
{
	MeshData MeshGirdData;
	uint32_t vertexNum = xPointNum * zPointNum;

	float halfLength = length * 0.5f;
	float halfWidth = width * 0.5f;
	
	float deltaZ = length / (zPointNum - 1);
	float deltaX = width / (xPointNum - 1);

	float deltaU = 1.0f / (xPointNum - 1);
	float deltaV = 1.0f / (zPointNum - 1);
	MeshGirdData.vertices.resize(vertexNum);

	for (uint32_t zi = 0; zi < zPointNum; zi++)
	{
		for (uint32_t xi = 0; xi < xPointNum; xi++)
		{
			Vertex girdVertex;

			girdVertex.position = XMFLOAT3(-halfWidth + xi * deltaX, 0.0f, halfLength - zi * deltaZ);
			girdVertex.textureUV.x = xi * deltaU;
			girdVertex.textureUV.y = 1.0f - zi * deltaV;

			girdVertex.tangent = XMFLOAT3(1.0f, 0.0f, 0.0f);
			girdVertex.normal = XMFLOAT3(0.0f, 1.0f, 0.0f);

			MeshGirdData.vertices[zi*xPointNum+xi]=girdVertex;
		}
	}

	for (int xi = 0; xi < xPointNum - 1; xi++)
	{
		for (int zi = 0; zi < zPointNum - 1; zi++)
		{
			MeshGirdData.indices32.push_back(zi * xPointNum + xi);
			MeshGirdData.indices32.push_back(zi * xPointNum + xi + 1);
			MeshGirdData.indices32.push_back((zi + 1) * xPointNum + xi);

			MeshGirdData.indices32.push_back(zi * xPointNum + xi + 1);
			MeshGirdData.indices32.push_back((zi + 1) * xPointNum + xi + 1);
			MeshGirdData.indices32.push_back((zi + 1) * xPointNum + xi);
		}
	}

	return MeshGirdData;
}

// 创建通过文件导入的模型
GeometryGenerator::MeshData GeometryGenerator::CreateImportedGeometryFromOBJ(const std::wstring& objPath)
{
	MeshData MeshObjData;

	std::wstring cachePath = objPath + L".bin";

	// -------- 优先加载缓存 --------
	std::ifstream finCache(cachePath, std::ios::binary);
	if (finCache)
	{
		UINT vcount = 0, icount = 0;
		finCache.read((char*)&vcount, sizeof(UINT));
		finCache.read((char*)&icount, sizeof(UINT));

		MeshObjData.vertices.resize(vcount);
		MeshObjData.indices32.resize(icount);

		finCache.read((char*)MeshObjData.vertices.data(), sizeof(Vertex) * vcount);
		finCache.read((char*)MeshObjData.indices32.data(), sizeof(uint32_t) * icount);
		finCache.close();
	}
	else
	{
		// -------- 从 OBJ 解析 --------
		std::ifstream fin(objPath);
		if (!fin)
		{
			MessageBox(nullptr, L"OBJ file not found.", nullptr, 0);
			return MeshObjData;
		}

		std::vector<DirectX::XMFLOAT3> positions, normals;
		std::vector<DirectX::XMFLOAT2> texcoords;

		std::string line;
		while (std::getline(fin, line))
		{
			std::stringstream ss(line);
			std::string prefix;
			ss >> prefix;
			if (prefix == "v") 
			{ 
				DirectX::XMFLOAT3 p; 
				ss >> p.x >> p.y >> p.z; 
				positions.push_back(p); 
			}
			else if (prefix == "vn") 
			{ 
				DirectX::XMFLOAT3 n; 
				ss >> n.x >> n.y >> n.z; 
				normals.push_back(n); 
			}
			else if (prefix == "vt") 
			{ 
				DirectX::XMFLOAT2 uv; 
				ss >> uv.x >> uv.y; 
				texcoords.push_back(uv); 
			}
			else if (prefix == "f")
			{
				std::string vStr[3];
				ss >> vStr[0] >> vStr[1] >> vStr[2];

				for (int i = 0; i < 3; ++i)
				{
					std::stringstream vs(vStr[i]);
					std::string idxStr; 
					int vIdx = 0, vtIdx = 0, vnIdx = 0;

					std::getline(vs, idxStr, '/'); vIdx = std::stoi(idxStr) - 1;
					std::getline(vs, idxStr, '/'); vtIdx = idxStr.empty() ? -1 : std::stoi(idxStr) - 1;
					std::getline(vs, idxStr, '/'); vnIdx = idxStr.empty() ? -1 : std::stoi(idxStr) - 1;

					Vertex vert = {};
					vert.position = positions[vIdx];
					vert.normal = (vnIdx >= 0) ? normals[vnIdx] : DirectX::XMFLOAT3(0, 1, 0);
					vert.tangent = DirectX::XMFLOAT3(0, 0, 0);
					vert.textureUV = (vtIdx >= 0) ? texcoords[vtIdx] : DirectX::XMFLOAT2(0, 0);

					MeshObjData.indices32.push_back((int)MeshObjData.vertices.size());
					MeshObjData.vertices.push_back(vert);
				}
			}
		}
		fin.close();

		// -------- 计算真实切线 --------
		ComputeTangents(MeshObjData.vertices, MeshObjData.indices32);

		// -------- 保存缓存 --------
		std::ofstream foutCache(cachePath, std::ios::binary);
		UINT vcount = (UINT)MeshObjData.vertices.size();
		UINT icount = (UINT)MeshObjData.indices32.size();
		foutCache.write((char*)&vcount, sizeof(UINT));
		foutCache.write((char*)&icount, sizeof(UINT));
		foutCache.write((char*)MeshObjData.vertices.data(), sizeof(Vertex) * vcount);
		foutCache.write((char*)MeshObjData.indices32.data(), sizeof(uint32_t) * icount);
		foutCache.close();
	}

	return MeshObjData;
}


void GeometryGenerator::ComputeTangents(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
{
	// 初始化切线
	for (auto& v : vertices)
		v.tangent = DirectX::XMFLOAT3(0, 0, 0);

	// 遍历每个三角形
	for (size_t i = 0; i < indices.size(); i += 3)
	{
		Vertex& v0 = vertices[indices[i + 0]];
		Vertex& v1 = vertices[indices[i + 1]];
		Vertex& v2 = vertices[indices[i + 2]];

		// 顶点位置
		DirectX::XMVECTOR p0 = DirectX::XMLoadFloat3(&v0.position);
		DirectX::XMVECTOR p1 = DirectX::XMLoadFloat3(&v1.position);
		DirectX::XMVECTOR p2 = DirectX::XMLoadFloat3(&v2.position);

		// 顶点 UV
		DirectX::XMVECTOR uv0 = DirectX::XMLoadFloat2(&v0.textureUV);
		DirectX::XMVECTOR uv1 = DirectX::XMLoadFloat2(&v1.textureUV);
		DirectX::XMVECTOR uv2 = DirectX::XMLoadFloat2(&v2.textureUV);

		float x1 = v1.position.x - v0.position.x;
		float y1 = v1.position.y - v0.position.y;
		float z1 = v1.position.z - v0.position.z;

		float x2 = v2.position.x - v0.position.x;
		float y2 = v2.position.y - v0.position.y;
		float z2 = v2.position.z - v0.position.z;

		float s1 = v1.textureUV.x - v0.textureUV.x;
		float t1 = v1.textureUV.y - v0.textureUV.y;
		float s2 = v2.textureUV.x - v0.textureUV.x;
		float t2 = v2.textureUV.y - v0.textureUV.y;

		float r = 1.0f / (s1 * t2 - s2 * t1);
		DirectX::XMFLOAT3 tangent;
		tangent.x = (t2 * x1 - t1 * x2) * r;
		tangent.y = (t2 * y1 - t1 * y2) * r;
		tangent.z = (t2 * z1 - t1 * z2) * r;

		// 累加到每个顶点
		v0.tangent.x += tangent.x; v0.tangent.y += tangent.y; v0.tangent.z += tangent.z;
		v1.tangent.x += tangent.x; v1.tangent.y += tangent.y; v1.tangent.z += tangent.z;
		v2.tangent.x += tangent.x; v2.tangent.y += tangent.y; v2.tangent.z += tangent.z;
	}

	// 归一化每个顶点的切线，并正交化（可选）
	for (auto& v : vertices)
	{
		DirectX::XMVECTOR t = DirectX::XMLoadFloat3(&v.tangent);
		DirectX::XMVECTOR n = DirectX::XMLoadFloat3(&v.normal);

		// 正交化切线：T = normalize(T - N * dot(N,T))
		t = DirectX::XMVector3Normalize(t - n * DirectX::XMVector3Dot(n, t));

		DirectX::XMStoreFloat3(&v.tangent, t);
	}
}

// 获取存有unit16_t类型的索引vector
std::vector<uint16_t>& GeometryGenerator::MeshData::getIndices16()
{
	if (indices16.empty())
	{
		indices16.resize(indices32.size());
		for (size_t i = 0; i < indices32.size(); i++)
			indices16[i] = static_cast<uint16_t>(indices32[i]);
	}

	return indices16;
}

// 获取存有unit32_t类型的索引vector
std::vector<uint32_t>& GeometryGenerator::MeshData::getIndices32()
{
	return indices32;
}