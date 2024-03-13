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
    float4x4 gWorld;//��������任����
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
    float4x4 gView;//�������ͼ����
    float4x4 gInvView;//��ͼ����������
    float4x4 gProj;//ͶӰ������ʾ��Ļ������
    float4x4 gInvProj;//ͶӰ����������
    float4x4 gViewProj;//��ͼͶӰ����
    float4x4 gInvViewProj;//��ͼͶӰ����������
    float3 gEyePosW;//�����λ������
    float cbPerObjectPad1;
    float2 gRenderTargetSize;//��ȾĿ��Ĵ�С
    float2 gInvRenderTargetSize;//��ȾĿ���С�ĵ���
    float gNearZ;//����ƽ��
    float gFarZ;//Զ��ƽ��
    float gTotalTime;//����������ʱ��
    float gDeltaTime;//����tick֮���ʱ���
    float4 gAmbientIlluminating;//����������
    
    Light gLights[MAX_NUM_LIGHTS];
}

struct VertexIn
{
	float3 pos	 :POSITION;
    float3 normal : NORMAL;
    //float2 texture : TEXTURE;
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