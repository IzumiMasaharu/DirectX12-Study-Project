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

cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;//物体世界变换矩阵
}
cbuffer cbMaterial : register(b1)
{
    float4 gDiffuseAlbedo;
    float3 gFresneRf0;
    float gRoughness;
    float4x4 gMaterialTransform;
}
cbuffer cbPass : register(b2)
{
    float4x4 gView;//摄像机视图矩阵
    float4x4 gInvView;//视图矩阵的逆矩阵
    float4x4 gProj;//投影（至显示屏幕）矩阵
    float4x4 gInvProj;//投影矩阵的逆矩阵
    float4x4 gViewProj;//视图投影矩阵
    float4x4 gInvViewProj;//视图投影矩阵的逆矩阵
    float3 gEyePosW;//摄像机位置坐标
    float cbPerObjectPad1;
    float2 gRenderTargetSize;//渲染目标的大小
    float2 gInvRenderTargetSize;//渲染目标大小的倒数
    float gNearZ;//近视平面
    float gFarZ;//远视平面
    float gTotalTime;//程序运行总时间
    float gDeltaTime;//两次tick之间的时间差
    float4 gAmbientIlluminating;//物体自身发光
    
    Light gLights[MAX_NUM_LIGHTS];
}

struct VertexIn
{
	float3 pos	 :POSITION;
    float3 normal : NORMAL;
};
struct VertexOut
{
    float4 posH : SV_Position;
    float3 posW : POSITIONT;
    float3 normalW : NORMAL;
};

VertexOut VS(VertexIn vin)
{   
    VertexOut vout;
    
    float4 pos = mul(float4(vin.pos, 1.0f), gWorld);
    vout.posW = pos.xyz;
    vout.normalW = mul(vin.normal, (float3x3) gWorld);
    vout.posH = mul(pos, gViewProj);
    
    return vout;
}

float4 PS( VertexOut pin ) : SV_Target
{   
    pin.normalW = normalize(pin.normalW);
    float3 toEyeW = normalize(gEyePosW - pin.posW);
    
    float4 ambientLight = gAmbientIlluminating * gDiffuseAlbedo;
    
    Material material = { gDiffuseAlbedo, gFresneRf0, gRoughness };
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeAllLights(gLights, material, pin.posW, pin.normalW, toEyeW, shadowFactor);
    
    float4 allLightIntensity = ambientLight + directLight;
    allLightIntensity.a = gDiffuseAlbedo.a;
    
	return allLightIntensity;
}