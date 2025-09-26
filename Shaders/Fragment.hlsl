#include "Basic.hlsl"

float4 PS(VertexOut pin) : SV_Target
{
	float4 diffuseAlbedo = gDiffuseMap.Sample(gsamAnisotropicWrap, pin.texCoord) * gDiffuseAlbedo;

	float3 binormal = normalize(cross(pin.normalW, pin.tangentW));
	float3x3 TBN = float3x3(pin.tangentW, binormal, pin.normalW);

	float3 normalTex = gNormalMap.Sample(gsamAnisotropicWrap, pin.texCoord).rgb;
	normalTex = normalize(normalTex * 2.0f - 1.0f);
	pin.normalW = normalize(mul(normalTex, TBN));

	float3 toEyeW = gEyePosW - pin.posW;
	float disToEye = length(toEyeW);
	toEyeW = normalize(toEyeW);

	float4 ambientLight = gAmbientIlluminating * gDiffuseAlbedo;

	Material material = { diffuseAlbedo, gFresneRf0, gRoughness };
	float3 shadowFactor = 1.0f;
	float4 directLight = ComputeAllLights(gLights, material, pin.posW, pin.normalW, toEyeW, shadowFactor);

	float4 allLightColor = ambientLight + directLight;

	float fogAlpha = saturate((disToEye - 1.0f) / 50.0f);
	allLightColor = lerp(allLightColor, float4(1.0f,1.0f,0.0f,1.0f), fogAlpha);

	allLightColor.a = gDiffuseAlbedo.a;

	return allLightColor;
}