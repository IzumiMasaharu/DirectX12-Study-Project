#include "Basic.hlsl"

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

	float4 pos = mul(float4(vin.pos, 1.0f), worldTransform);
	vout.posW = pos.xyz;
	vout.normalW = normalize(mul(vin.normal, (float3x3)normalMatrix));
	vout.tangentW = normalize(mul(vin.tangent, (float3x3)worldTransform));
	vout.posH = mul(pos, gViewProj);
	vout.texCoord = mul(float4(vin.texCoord, 0.0f, 1.0f), gTextureTransform).xy;

	return vout;
}