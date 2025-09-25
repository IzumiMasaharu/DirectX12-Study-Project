#include "Basic.hlsl"

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

	// ==== Tangent ====
	float3 approxTangent = float3(1.0f, 0.0f, 0.0f);
	float3 tangent = normalize(approxTangent - normal * dot(normal, approxTangent));
	vout.tangentW = mul(tangent, (float3x3)gWorldTransform);

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