#include "Basic.hlsl"

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

	float4 pos = mul(float4(vin.pos, 1.0f), gWorldTransform);
	vout.posW = pos.xyz;
	vout.normalW = mul(vin.normal, (float3x3) gWorldTransform);
	vout.tangentW = mul(vin.tangentU, (float3x3) gWorldTransform);
	vout.posH = mul(pos, gViewProj);
	float4 texC = mul(float4(vin.texCoord, 0.0f, 1.0f), gTextureTransform);
	vout.texCoord = mul(texC, gMaterialTransform).xy;

	return vout;
}