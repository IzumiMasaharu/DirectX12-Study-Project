#include "Basic.hlsl"

float4 PS(VertexOut pin) : SV_Target
{
	Texture2D diffuseMap = gTextures[diffuseTextureIndex[0]];
	Texture2D normalMap = gTextures[normalTextureIndex[0]];
	MaterialData material = materialBuffer[materialIndex];

	material.albedo = diffuseMap.Sample(gsamAnisotropicWrap, pin.texCoord) * material.albedo;

	float3 binormal = normalize(cross(pin.normalW, pin.tangentW));
	float3x3 TBN = float3x3(pin.tangentW, binormal, pin.normalW);

	float3 normalTex = normalMap.Sample(gsamAnisotropicWrap, pin.texCoord).rgb;
	normalTex = normalize(normalTex * 2.0f - 1.0f);
	pin.normalW = normalize(mul(normalTex, TBN));

	float3 toEyeW = gEyePosW - pin.posW;
	float disToEye = length(toEyeW);
	toEyeW = normalize(toEyeW);

	float4 ambientLight = gAmbientIlluminating * material.albedo;
	float3 shadowFactor = 1.0f;
	float4 directLight = ComputeAllLights(gLights, material, pin.posW, pin.normalW, toEyeW, shadowFactor);

	float4 allLightColor = ambientLight + directLight;

	// float fogAlpha = saturate((disToEye - 1.0f) / 50.0f);
	// allLightColor = lerp(allLightColor, float4(1.0f,1.0f,0.0f,1.0f), fogAlpha);

	allLightColor.a = material.albedo.a;

	return allLightColor;
}