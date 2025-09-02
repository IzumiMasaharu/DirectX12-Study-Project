#ifndef NUM_DIRECTIONAL_LIGHTS
#define NUM_DIRECTIONAL_LIGHTS 0
#endif
#ifndef NUM_POINT_LIGHTS
#define NUM_POINT_LIGHTS 1
#endif
#ifndef NUM_SPOT_LIGHTS
#define NUM_SPOT_LIGHTS 0
#endif

#include "Light.hlsl"

Texture2D gDiffuseMap : register(t0);

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorldTransform; // 物体世界变换矩阵
	float4x4 gTextureTransform; // 纹理变换矩阵
}
cbuffer cbMaterial : register(b1)
{
	float4 gDiffuseAlbedo; // 漫反射反照率
	float3 gFresneRf0; // 菲涅尔效应材质属性Rf（0°）
	float gRoughness; // 材质粗糙度
	float4x4 gMaterialTransform;
}
cbuffer cbPass : register(b2)
{
	float4x4 gView; // 摄像机视图矩阵
	float4x4 gInvView; // 视图矩阵的逆矩阵
	float4x4 gProj; // 投影（至显示屏幕）矩阵
	float4x4 gInvProj; // 投影矩阵的逆矩阵
	float4x4 gViewProj; // 视图投影矩阵
	float4x4 gInvViewProj; // 视图投影矩阵的逆矩阵
	float3 gEyePosW; // 摄像机位置坐标
	float cbPerObjectPad1; // 填充字节以保证16字节对齐
	float2 gRenderTargetSize; // 渲染目标的大小
	float2 gInvRenderTargetSize; // 渲染目标大小的倒数
	float gNearZ; // 近视平面
	float gFarZ; // 远视平面
	float gTotalTime; // 程序运行总时间
	float gDeltaTime; // 两次tick之间的时间差
	float4 gAmbientIlluminating;//物体自身发光

	Light gLights[MAX_NUM_LIGHTS];
}

// 输入顶点数据
struct VertexIn
{
	float3 pos      : POSITION;
	float3 normal   : NORMAL;
	float2 texCoord : TEXCOORD;
};
// 输出顶点数据
struct VertexOut
{
	float4 posH     : SV_POSITION;
	float3 posW     : POSITION;
	float3 normalW  : NORMAL;
	float2 texCoord : TEXCOORD;
};

// 顶点着色器
VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

	float4 pos = mul(float4(vin.pos, 1.0f), gWorldTransform);
	vout.posW = pos.xyz;
	vout.normalW = mul(vin.normal, (float3x3) gWorldTransform);
	vout.posH = mul(pos, gViewProj);
	float4 texC = mul(float4(vin.texCoord, 0.0f, 1.0f), gTextureTransform);
	vout.texCoord = mul(texC, gMaterialTransform).xy;

	return vout;
}

VertexOut VS_Wave(VertexIn vin)
{
	VertexOut vout;

	// 世界空间位置
	float4 posW = mul(float4(vin.pos, 1.0f), gWorldTransform);

	// ==== 多层叠加波浪 ====
	// 每层波浪有不同的方向、频率、幅度、速度
	float waveHeight = 0.0f;

	float2 waveDirs[4] = {
		float2(1.0, 0.3),
		float2(0.6, 1.0),
		float2(0.8, 0.8),
		float2(1.0, -0.5)
	};

	float waveLengths[4] = { 4.0f, 2.5f, 3.0f, 5.0f };
	float amplitudes[4] = { 0.15f, 0.08f, 0.12f, 0.1f };
	float speeds[4] = { 1.5f, 2.0f, 1.0f, 1.2f };

	for (int i = 0; i < 4; ++i)
	{
		float k = 2 * 3.1415926 / waveLengths[i]; // 波数
		float phase = dot(posW.xz, waveDirs[i]) * k + gTotalTime * speeds[i];
		waveHeight += amplitudes[i] * sin(phase);
	}

	// 叠加波浪
	posW.y += waveHeight;

	// ==== 法线扰动 ====
	// 用有限差分近似法线偏导
	float epsilon = 0.01f;
	float heightX = 0.0f, heightZ = 0.0f;
	for (int i = 0; i < 4; ++i)
	{
		float k = 2 * 3.1415926 / waveLengths[i];
		float phaseX = dot(posW.xz + float2(epsilon, 0), waveDirs[i]) * k + gTotalTime * speeds[i];
		float phaseZ = dot(posW.xz + float2(0, epsilon), waveDirs[i]) * k + gTotalTime * speeds[i];
		heightX += amplitudes[i] * sin(phaseX);
		heightZ += amplitudes[i] * sin(phaseZ);
	}
	float dx = (heightX - waveHeight) / epsilon;
	float dz = (heightZ - waveHeight) / epsilon;
	float3 normal = normalize(float3(-dx, 1.0f, -dz));
	vout.normalW = mul(normal, (float3x3)gWorldTransform);

	// 输出裁剪空间
	vout.posH = mul(posW, gViewProj);

	// 世界坐标
	vout.posW = posW.xyz;

	// ==== UV 流动 ====
	vout.texCoord = vin.texCoord;

	// 主流方向
	float2 flowDir = float2(1.0, 0.3);
	float flowSpeed = 0.05;
	vout.texCoord += flowDir * gTotalTime * flowSpeed;

	// 小幅随机扰动模拟涟漪
	float waveUV = 0.02 * (sin(5.0 * posW.x + gTotalTime * 3.0) + cos(4.0 * posW.z + gTotalTime * 2.0));
	vout.texCoord += waveUV;

	return vout;
}

// 像素着色器
float4 PS(VertexOut pin) : SV_Target
{
	float4 diffuseAlbedo = gDiffuseMap.Sample(gsamAnisotropicWrap, pin.texCoord) * gDiffuseAlbedo;
	pin.normalW = normalize(pin.normalW);
	float3 toEyeW = normalize(gEyePosW - pin.posW);

	float4 ambientLight = gAmbientIlluminating * gDiffuseAlbedo;

	Material material = { diffuseAlbedo, gFresneRf0, gRoughness };
	float3 shadowFactor = 1.0f;
	float4 directLight = ComputeAllLights(gLights, material, pin.posW, pin.normalW, toEyeW, shadowFactor);

	float4 allLightColor = ambientLight + directLight;
	allLightColor.a = gDiffuseAlbedo.a;

	return allLightColor;
}