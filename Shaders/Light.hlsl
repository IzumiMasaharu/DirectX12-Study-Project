#define MAX_NUM_LIGHTS 256

#include "MaterialData.hlsl"

struct Light
{
    float3 rgbIntensity;
    float start;
    float3 direction;
    float end;
    float3 position;
    float spotPower;
};

// 光学计算方法
// 计算线性衰减因子
float CalculatorAttenuatiohn(float d, float start, float end)
{
    return saturate((end - d) / (end - start));
}
// 石里克近似模拟菲涅尔反射率
float SchlickFresnel(float3 Rf0, float3 normal, float3 lightVector)
{
    float cosNormalLightAngle = saturate(dot(normal, lightVector));
    float r = 1.0f - cosNormalLightAngle;
    float3 reflectPercent = Rf0 + (1.0f - Rf0) * pow(r, 5);
    
    return reflectPercent;
}
// Schlick-GGX 几何遮蔽函数
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float k = (roughness * roughness) / 2.0f;
    return NdotV / (NdotV * (1.0f - k) + k);
}
// 几何遮蔽项 G
float GeometrySmith(float3 normal, float3 viewDir, float3 lightDir, float roughness)
{
    float NdotV = max(dot(normal, viewDir), 0.0f);
    float NdotL = max(dot(normal, lightDir), 0.0f);
    float ggx1 = GeometrySchlickGGX(NdotV, roughness);
    float ggx2 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}
// 计算因漫反射与镜面反射而进入人眼的光量
float3 reflectedLightColor(float3 rgbIntensity, float3 lightVector, float3 normal, float3 toEyeVector, MaterialData material)
{
    const float m = (1.0f - material.roughness) * 256.0f;
    float3 halfVector = normalize(toEyeVector + lightVector);

    // 法线分布项（D）
    float roughnessFactor = ((m + 8.0f) / 8.0f) * pow(max(dot(halfVector, normal), 0.0f), m);

    // 菲涅尔反射率（F）
    float3 fresnelFactor = SchlickFresnel(material.fresnel, halfVector, lightVector);

    // 几何遮蔽项（G）
    float geometryFactor = GeometrySmith(normal, toEyeVector, lightVector, material.roughness);

    // 结合 D、F、G
    float3 mirrorReflectedAlbedo = (roughnessFactor * fresnelFactor * geometryFactor);
    mirrorReflectedAlbedo = mirrorReflectedAlbedo / (mirrorReflectedAlbedo + 1.0f);

    return (mirrorReflectedAlbedo + material.albedo.rgb) * rgbIntensity;
}

// 光源生成方法
// 生成平行光
float3 ComputeDirectionalLight(Light light,MaterialData material,float3 normal,float3 toEyeVector)
{
    float3 lightVector = -light.direction;
    float3 lightIntensity = light.rgbIntensity * max(dot(lightVector, normal), 0.0f);
    
    return reflectedLightColor(lightIntensity, lightVector, normal, toEyeVector, material);
}

// 生成点光源
float3 ComputePointLight(Light light, MaterialData material,float3 illuminatedPosition,float3 normal,float3 toEyeVector)
{
    float3 lightVector = light.position - illuminatedPosition;
    if (length(lightVector)>light.end)
        return 0.0f;
    lightVector /= length(lightVector);
    
    float3 lightIntensity = light.rgbIntensity;
    lightIntensity *= CalculatorAttenuatiohn(length(lightVector), light.start, light.end);
    lightIntensity *= max(dot(lightVector, normal), 0.0f);

    return reflectedLightColor(lightIntensity, lightVector, normal, toEyeVector, material);
}

// 生成聚光灯
float3 ComputeSpotLight(Light light, MaterialData material,float3 illuminatedPosition,float3 normal,float3 toEyeVector)
{
    float3 lightVector = -(illuminatedPosition - light.position);
    if (length(lightVector) > light.end)
        return 0.0f;
    lightVector /= length(lightVector);
    
    float3 lightIntensity = light.rgbIntensity;
    lightIntensity *= max(dot(lightVector, normal), 0.0f);
    lightIntensity *= pow(max(dot(-lightVector, light.direction), 0.0f), light.spotPower);
    lightIntensity *= CalculatorAttenuatiohn(length(lightVector), light.start, light.end);
    
    return reflectedLightColor(lightIntensity, lightVector, normal, toEyeVector, material);
}

// 生成全部光源
float4 ComputeAllLights(Light lights[MAX_NUM_LIGHTS], MaterialData material,float3 illuminatedPosition,float3 normal,float3 toEyeVector,float3 shadowFactor)
{
    float3 result = 0.0f;
    int index = 0;

#if (NUM_DIRECTIONAL_LIGHTS > 0)
    for(index = 0; index < NUM_DIRECTIONAL_LIGHTS; ++index)
        result += shadowFactor[index] * ComputeDirectionalLight(lights[index], material, normal, toEyeVector);
#endif

#if (NUM_POINT_LIGHTS > 0)
    for(index = NUM_DIRECTIONAL_LIGHTS; index < NUM_DIRECTIONAL_LIGHTS+NUM_POINT_LIGHTS; ++index)
        result += ComputePointLight(lights[index], material, illuminatedPosition, normal, toEyeVector);
#endif

#if (NUM_SPOT_LIGHTS > 0)
    for(index = NUM_DIRECTIONAL_LIGHTS + NUM_POINT_LIGHTS; index < NUM_DIRECTIONAL_LIGHTS + NUM_POINT_LIGHTS + NUM_SPOT_LIGHTS; ++index)
        result += ComputeSpotLight(lights[index], material, illuminatedPosition, normal, toEyeVector);
#endif 

    return float4(result, 0.0f);
}


