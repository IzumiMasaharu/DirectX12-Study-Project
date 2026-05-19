#include "Basic.hlsl"

VertexOut VS(VertexIn vin, uint instanceID : SV_InstanceID)
{
	VertexOut vout = (VertexOut)0.0f;

	InstanceData instance = instanceDataBuffer[instanceID];
	float4x4 worldTransform = instance.worldTransform;
	float4x4 normalMatrix = instance.normalMatrix;
	float4x4 textureTransform = instance.textureTransform;
	uint materialIndex = instance.materialIndex;

	vout.materialIndex = materialIndex;
	vout.textureTableIndex = instance.textureTableIndex;
	vout.textureFlags = instance.textureFlags;

	MaterialData materialData = materialDataBuffer[materialIndex];

	float4 pos = mul(float4(vin.pos, 1.0f), worldTransform);
	vout.posW = pos.xyz;
	vout.normalW = normalize(mul(vin.normal, (float3x3)normalMatrix));
	vout.tangentW = normalize(mul(vin.tangent, (float3x3)worldTransform));
	vout.posH = mul(pos, gViewProj);
	float4 texCoord = mul(float4(vin.texCoord, 0.0f, 1.0f), materialData.materialTransform);
	vout.texCoord = mul(texCoord, textureTransform).xy;

	return vout;
}
