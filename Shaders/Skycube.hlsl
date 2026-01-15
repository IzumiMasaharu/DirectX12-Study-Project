#include "Basic.hlsl"

TextureCube gSkycubeMap : register(t0, space2);
// 输入顶点数据
struct SkycubeVertexIn
{
	float3 pos      : POSITION;
	float3 normal   : NORMAL;
	float2 texCoord : TEXCOORD;
};
// 输出顶点数据
struct SkycubeVertexOut
{
	float4 posH     : SV_POSITION;	// 屏幕空间坐标
	float3 posL     : POSITION;		// 局部空间坐标
};

SkycubeVertexOut SkycubeVS(SkycubeVertexIn vin)
{
	SkycubeVertexOut vout = (SkycubeVertexOut)0.0f;

	vout.posL = vin.pos;
	float4 posW = mul(float4(vin.pos, 1.0f), worldTransform);
	posW.xyz += gEyePosW;	// 让天空盒跟随摄像机位置移动
	vout.posH = mul(posW, gViewProj).xyww;

	return vout;
}
float4 SkycubePS(SkycubeVertexOut pin) : SV_TARGET
{
	return gSkycubeMap.Sample(gsamLinearClamp, pin.posL);
}