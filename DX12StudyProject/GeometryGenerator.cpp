#include "GeometryGenerator.h"

using namespace DirectX;

void GeometryGenerator::CreateCylinderTop(float topRadius, float height, uint32_t sliceCount, MeshData& meshData)const
{
	auto baseIndex = (uint32_t)meshData.Vertices.size();

	float y = 0.5f * height;
	float dTheta = (2.0f * XM_PI) / sliceCount;

	for (uint32_t i = 0; i <= sliceCount; i++)
	{
		float x = topRadius * cosf(i * dTheta);
		float z = topRadius * sinf(i * dTheta);

		float u = x / height + 0.5f;
		float v = z / height + 0.5f;

		meshData.Vertices.push_back(Vertex(x, y, z, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v));
	}

	meshData.Vertices.push_back(Vertex(0.0f, y, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f));

	for (uint32_t i = 0; i < sliceCount; i++)
	{
		meshData.Indices_32.push_back((uint32_t)meshData.Vertices.size() - 1);
		meshData.Indices_32.push_back(baseIndex + i + 1);
		meshData.Indices_32.push_back(baseIndex + i);
	}
}
void GeometryGenerator::CreateCylinderBottom(float bottomRadius, float height, uint32_t sliceCount, MeshData& meshData)const
{
	auto baseIndex = (uint32_t)meshData.Vertices.size();

	float y = -0.5f * height;
	float dTheta = (2.0f * XM_PI) / sliceCount;

	for (uint32_t i = 0; i <= sliceCount; i++)
	{
		float x = bottomRadius * cosf(i * dTheta);
		float z = bottomRadius * sinf(i * dTheta);

		float u = x / height + 0.5f;
		float v = z / height + 0.5f;

		meshData.Vertices.push_back(Vertex(x, y, z, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v));
	}

	meshData.Vertices.push_back(Vertex(0.0f, y, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f));

	for (uint32_t i = 0; i < sliceCount; i++)
	{
		meshData.Indices_32.push_back((uint32_t)meshData.Vertices.size() - 1);
		meshData.Indices_32.push_back(baseIndex + i);
		meshData.Indices_32.push_back(baseIndex + i+1);
	}
}
//生成圆台
GeometryGenerator::MeshData GeometryGenerator::CreateCylinder(float bottomRadius, float topRadius, float height, uint32_t sliceCount, uint32_t stackCount)const
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
			vertex.Texture.x = (float)j / sliceCount;
			vertex.Texture.y = 1.0f-(float)i / stackCount;

			vertex.Tangent = XMFLOAT3(-sinf(theta), 0.0f, cosf(theta));

			float dR = bottomRadius - topRadius;
			XMFLOAT3 bitangent(dR * cosf(theta), -height, dR * sinf(theta));
			XMVECTOR T = XMLoadFloat3(&vertex.Tangent);
			XMVECTOR B = XMLoadFloat3(&bitangent);
			XMVECTOR N = XMVector3Normalize(XMVector3Cross(T, B));
			XMStoreFloat3(&vertex.Normal, N);

			MeshCylinderData.Vertices.push_back(vertex);
		}
	}

	uint32_t VertexPerRing = sliceCount + 1;
	for (uint32_t i = 0; i < stackCount; i++)
	{
		for (uint32_t j = 0; j < sliceCount; j++)
		{
			MeshCylinderData.Indices_32.push_back(i * VertexPerRing + j);
			MeshCylinderData.Indices_32.push_back((i + 1) * VertexPerRing + j);
			MeshCylinderData.Indices_32.push_back((i + 1) * VertexPerRing + j + 1);

			MeshCylinderData.Indices_32.push_back(i * VertexPerRing + j);
			MeshCylinderData.Indices_32.push_back((i + 1) * VertexPerRing + j + 1);
			MeshCylinderData.Indices_32.push_back(i * VertexPerRing + j + 1);
		}
	}

	CreateCylinderTop(topRadius, height, sliceCount, MeshCylinderData);
	CreateCylinderBottom(bottomRadius, height, sliceCount, MeshCylinderData);

	return MeshCylinderData;
}
//生成球体
GeometryGenerator::MeshData GeometryGenerator::CreateBall(float Radius, uint32_t sliceCount, uint32_t stackCount)const
{
	MeshData MeshBallData;

	Vertex topVertex(0.0f, +Radius, 0.0f, 0.0f, +1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	Vertex bottomVertex(0.0f, -Radius, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

	MeshBallData.Vertices.push_back(topVertex);

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
			v.Tangent.x = -Radius * sinf(phi) * sinf(theta);
			v.Tangent.y = 0.0f;
			v.Tangent.z = +Radius * sinf(phi) * cosf(theta);

			XMVECTOR T = XMLoadFloat3(&v.Tangent);
			XMStoreFloat3(&v.Tangent, XMVector3Normalize(T));

			XMVECTOR p = XMLoadFloat3(&v.position);
			XMStoreFloat3(&v.Normal, XMVector3Normalize(p));

			v.Texture.x = theta / XM_2PI;
			v.Texture.y = phi / XM_PI;

			MeshBallData.Vertices.push_back(v);
		}
	}

	MeshBallData.Vertices.push_back(bottomVertex);

	for (uint32_t i = 1; i <= sliceCount; ++i)
	{
		MeshBallData.Indices_32.push_back(0);
		MeshBallData.Indices_32.push_back(i + 1);
		MeshBallData.Indices_32.push_back(i);
	}

	uint32_t baseIndex = 1;
	uint32_t ringVertexCount = sliceCount + 1;
	for (uint32_t i = 0; i < stackCount - 2; i++)
	{
		for (uint32_t j = 0; j < sliceCount; j++)
		{
			MeshBallData.Indices_32.push_back(baseIndex + i * ringVertexCount + j);
			MeshBallData.Indices_32.push_back(baseIndex + i * ringVertexCount + j + 1);
			MeshBallData.Indices_32.push_back(baseIndex + (i + 1) * ringVertexCount + j);
		
			MeshBallData.Indices_32.push_back(baseIndex + (i + 1) * ringVertexCount + j);
			MeshBallData.Indices_32.push_back(baseIndex + i * ringVertexCount + j + 1);
			MeshBallData.Indices_32.push_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
		}
	}

	uint32_t southPoleIndex = (uint32_t)MeshBallData.Vertices.size() - 1;

	baseIndex = southPoleIndex - ringVertexCount;

	for (uint32_t i = 0; i < sliceCount; i++)
	{
		MeshBallData.Indices_32.push_back(southPoleIndex);
		MeshBallData.Indices_32.push_back(baseIndex + i);
		MeshBallData.Indices_32.push_back(baseIndex + i + 1);
	}

	return MeshBallData;
}
//生成网格平面
GeometryGenerator::MeshData GeometryGenerator::CreateGird(float length, float width, uint32_t xPointNum, uint32_t zPointNum)const
{
	MeshData MeshGirdData;
	uint32_t vertexNum = xPointNum * zPointNum;

	float halfLength = length * 0.5f;
	float halfWidth = width * 0.5f;
	
	float deltaZ = length / (zPointNum - 1);
	float deltaX = width / (xPointNum - 1);

	float deltaU = 1.0f / (xPointNum - 1);
	float deltaV = 1.0f / (zPointNum - 1);
	MeshGirdData.Vertices.resize(vertexNum);

	for (uint32_t zi = 0; zi < zPointNum; zi++)
	{
		for (uint32_t xi = 0; xi < xPointNum; xi++)
		{
			Vertex girdVertex;

			girdVertex.position = XMFLOAT3(-halfWidth + xi * deltaX, 0.0f, halfLength - zi * deltaZ);
			girdVertex.Texture.x = xi * deltaU;
			girdVertex.Texture.y = 1.0f - zi * deltaV;

			girdVertex.Tangent = XMFLOAT3(1.0f, 0.0f, 0.0f);
			girdVertex.Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);

			MeshGirdData.Vertices.push_back(girdVertex);
		}
	}

	for (int xi = 0; xi < xPointNum - 1; xi++)
	{
		for (int zi = 0; zi < zPointNum - 1; zi++)
		{
			MeshGirdData.Indices_32.push_back(zi * xPointNum + xi);
			MeshGirdData.Indices_32.push_back(zi * xPointNum + xi + 1);
			MeshGirdData.Indices_32.push_back((zi + 1) * xPointNum + xi);

			MeshGirdData.Indices_32.push_back(zi * xPointNum + xi + 1);
			MeshGirdData.Indices_32.push_back((zi + 1) * xPointNum + xi + 1);
			MeshGirdData.Indices_32.push_back((zi + 1) * xPointNum + xi);
		}
	}

	return MeshGirdData;
}

//获取存有unit16_t类型的索引vector
std::vector<uint16_t>& GeometryGenerator::MeshData::GetIndices_16()
{
	if (mIndices_16.empty())
	{
		mIndices_16.resize(Indices_32.size());
		for (size_t i = 0; i < Indices_32.size(); i++)
			mIndices_16[i] = static_cast<uint16_t>(Indices_32[i]);
	}

	return mIndices_16;
}