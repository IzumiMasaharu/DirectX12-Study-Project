#define MAX_NUM_LIGHTS 256

#include "Basic.hlsl"

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
    float3 schlickFresnel = Rf0 + (1.0f - Rf0) * pow(r, 5);
    
    return schlickFresnel;
}
// GTR1 法线分布函数
float GTR1(float3 normal,float3 halfVector, float alpha)
{
    float NdotH = max(dot(normal, halfVector), 0.0f);
    float alpha2 = alpha * alpha;
    float t = 1 + (alpha2-1) * NdotH * NdotH;
    return (alpha2 - 1) / (3.14159 * log(alpha2) * t);
}
// GTR2 法线分布函数
float GTR2(float3 normal,float3 halfVector, float alpha)
{
    float NdotH = max(dot(normal, halfVector), 0.0f);
    float alpha2 = alpha * alpha;
    float t = 1 + (alpha2-1) * NdotH * NdotH;
    return alpha2 / (3.14159 * t * t);
}
// Schlick-GGX 几何遮蔽函数
float GeometrySchlickGGX(float XdotY, float roughness)
{
    float k = ( 0.5 + roughness / 2.0f) * (0.5 + roughness / 2.0f) / 2.0f;
    return XdotY / (XdotY * (1.0f - k) + k);
}
// Disney-Smith 几何遮蔽函数
float GeometryDisneySmithGGX(float XdotY, float roughness)
{
    float k = ( 0.5 + roughness / 2.0f) * (0.5 + roughness / 2.0f);
    return (2 * XdotY) / (XdotY + sqrt( k*k + (1.0f - k * k) * XdotY * XdotY));
}
// 几何遮蔽项 G
float GeometrySmith(float3 normal, float3 viewDir, float3 lightDir, float roughness)
{
    float NdotV = max(dot(normal, viewDir), 0.0f);
    float NdotL = max(dot(normal, lightDir), 0.0f);
    float ggx1 = GeometryDisneySmithGGX(NdotV, roughness);
    float ggx2 = GeometryDisneySmithGGX(NdotL, roughness);
    return ggx1 * ggx2;
}
// 计算因漫反射与镜面反射而进入人眼的光量
float3 reflectedLightColor(float3 rgbIntensity, float3 lightVector, float3 normal, float3 toEyeVector, MaterialData material)
{
    float3 halfVector = normalize(toEyeVector + lightVector);

    // 菲涅尔反射率（F）
    float3 fresnelFactor = SchlickFresnel(material.fresnel, halfVector, lightVector);
    // 法线分布项（D）
    float roughnessFactor = GTR2(normal, halfVector, material.roughness);
    // 几何遮蔽项（G）
    float geometryFactor = GeometrySmith(normal, toEyeVector, lightVector, material.roughness);
    // 结合 D、F、G  实现Cook-Torrance模型
    float3 mirrorReflectedAlbedo = (roughnessFactor * fresnelFactor * geometryFactor);
    mirrorReflectedAlbedo = mirrorReflectedAlbedo / (4* max(dot(normal, toEyeVector), 0.0f) * max(dot(normal, lightVector), 0.0f) + 0.001f);

    // Disney-Diffuse漫反射模型
    float fd90 = 0.5 + 2.0 * material.roughness * pow(1.0 - max(dot(normal, toEyeVector), 0.0f), 2.0);
    float lightScatter = 1.0 + (fd90 - 1.0) * pow(1.0 - max(dot(normal, lightVector), 0.0f), 5.0);
    float viewScatter = 1.0 + (fd90 - 1.0) * pow(1.0 - max(dot(normal, toEyeVector), 0.0f), 5.0);
    float3 diffuseAlbedo = material.albedo.rgb / 3.14159;
    float3 diffuseComponent = diffuseAlbedo * lightScatter * viewScatter;

    return rgbIntensity * (diffuseComponent + mirrorReflectedAlbedo);
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
    for(index = NUM_DIRECTIONAL_LIGHTS; index < NUM_DIRECTIONAL_LIGHTS + NUM_POINT_LIGHTS; ++index)
        result += ComputePointLight(lights[index], material, illuminatedPosition, normal, toEyeVector);
#endif

#if (NUM_SPOT_LIGHTS > 0)
    for(index = NUM_DIRECTIONAL_LIGHTS + NUM_POINT_LIGHTS; index < NUM_DIRECTIONAL_LIGHTS + NUM_POINT_LIGHTS + NUM_SPOT_LIGHTS; ++index)
        result += ComputeSpotLight(lights[index], material, illuminatedPosition, normal, toEyeVector);
#endif 

    return float4(result, 0.0f);
}


