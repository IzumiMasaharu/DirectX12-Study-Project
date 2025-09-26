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

TextureCube gSkycubeMap : register(t0);
Texture2D gDiffuseMap : register(t1);
Texture2D gNormalMap : register(t2);
Texture2D gHeightMap : register(t3);

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorldTransform;		// 物体世界变换矩阵
	float4x4 gTextureTransform;		// 纹理变换矩阵
}
cbuffer cbMaterial : register(b1)
{
	float4 gDiffuseAlbedo;			// 漫反射反照率
	float3 gFresneRf0;				// 菲涅尔效应材质属性Rf（0°）
	float gRoughness;				// 材质粗糙度
	float4x4 gMaterialTransform;
}
cbuffer cbPass : register(b2)
{
	float4x4 gView;					// 摄像机视图矩阵
	float4x4 gInvView;				// 视图矩阵的逆矩阵
	float4x4 gProj;					// 投影（至显示屏幕）矩阵
	float4x4 gInvProj;				// 投影矩阵的逆矩阵
	float4x4 gViewProj;				// 视图投影矩阵
	float4x4 gInvViewProj;			// 视图投影矩阵的逆矩阵
	float3 gEyePosW;				// 摄像机位置坐标
	float cbPerObjectPad1;			// 填充字节以保证16字节对齐
	float2 gRenderTargetSize;		// 渲染目标的大小
	float2 gInvRenderTargetSize;	// 渲染目标大小的倒数
	float gNearZ;					// 近视平面
	float gFarZ;					// 远视平面
	float gTotalTime;				// 程序运行总时间
	float gDeltaTime;				// 两次tick之间的时间差
	float4 gAmbientIlluminating;	// 物体自身发光

	Light gLights[MAX_NUM_LIGHTS];
}

// 输入顶点数据
struct VertexIn
{
	float3 pos      : POSITION;
	float3 normal   : NORMAL;
	float3 tangentU : TANGENT;
	float2 texCoord : TEXCOORD;
};
// 输出顶点数据
struct VertexOut
{
	float4 posH     : SV_POSITION;	// 屏幕空间坐标
	float3 posW     : POSITION;		// 世界空间坐标
	float3 normalW  : NORMAL;
	float3 tangentW : TANGENT;
	float2 texCoord : TEXCOORD;
};