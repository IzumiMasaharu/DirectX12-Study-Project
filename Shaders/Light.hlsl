#define MAX_NUM_LIGHTS 8

struct Light
{
    float3 rgbIntensity;
    float start;
    float3 direction;
    float end;
    float3 position;
    float spotPower;
};
struct Material
{
    float4 diffuseAlbedo;
    float3 fresneRf0;
    float roughness;
};

//��ѧ���㷽��
//��������˥������
float CalculatorAttenuatiohn(float d, float start, float end)
{
    return saturate((end - d) / (end - start));
}
//ʯ��˽���ģ�������������
float SchlickFresnel(float3 Rf0, float3 normal, float3 lightVector)
{
    float cosNormalLightAngle = saturate(dot(normal, lightVector));
    float r = 1.0f - cosNormalLightAngle;
    float3 reflectPercent = Rf0 + (1.0f - Rf0) * pow(r, 5);
    return reflectPercent;
}
//�������������뾵�淴����������۵Ĺ���
float3 reflectedLightColor(float3 rgbIntensity, float3 lightVector, float3 normal, float3 toEyeVector, Material material)
{
    const float m = (1.0f - material.roughness) * 256.0f;
    float3 halfVector = normalize(toEyeVector + lightVector);
    
    float roughnessFactor = ((m + 8.0f) / 8.0f) * pow(max(dot(halfVector, normal), 0.0f), m);
    float3 fresneFactor = SchlickFresnel(material.fresneRf0, halfVector, lightVector);
    float3 mirrorReflectedAlbedo = roughnessFactor * fresneFactor;
    mirrorReflectedAlbedo = mirrorReflectedAlbedo / (mirrorReflectedAlbedo + 1.0f);
    
    return (mirrorReflectedAlbedo + material.diffuseAlbedo.rgb) * rgbIntensity;
}

//��Դ���ɷ���
//����ƽ�й�
float3 ComputeDirectionalLight(Light light,Material material,float3 normal,float3 toEyeVector)
{
    float3 lightVector = -light.direction;
    float3 lightIntensity = light.rgbIntensity * max(dot(lightVector, normal), 0.0f);
    
    return reflectedLightColor(lightIntensity, lightVector, normal, toEyeVector, material);
}
//���ɵ��Դ
float3 ComputePointLight(Light light,Material material,float3 illuminatedPosition,float3 normal,float3 toEyeVector)
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
//���ɾ۹��
float3 ComputeSpotLight(Light light,Material material,float3 illuminatedPosition,float3 normal,float3 toEyeVector)
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

//����ȫ����Դ
float4 ComputeAllLights(Light lights[MAX_NUM_LIGHTS],Material material,float3 illuminatedPosition,float3 normal,float3 toEyeVector,float3 shadowFactor)
{
    float3 result = 0.0f;

    int index = 0;

#if (NUM_DIRECTIONAL_LIGHTS > 0)
    for(index = 0; index < NUM_DIRECTIONAL_LIGHTS; ++index)
    {
        result += shadowFactor[index] * ComputeDirectionalLight(lights[index], material, normal, toEyeVector);
    }
#endif

#if (NUM_POINT_LIGHTS > 0)
    for(index = NUM_DIRECTIONAL_LIGHTS; index < NUM_DIRECTIONAL_LIGHTS+NUM_POINT_LIGHTS; ++index)
    {
        result += ComputePointLight(lights[index], material, illuminatedPosition, normal, toEyeVector);
    }
#endif

#if (NUM_SPOT_LIGHTS > 0)
    for(index = NUM_DIRECTIONAL_LIGHTS + NUM_POINT_LIGHTS; index < NUM_DIRECTIONAL_LIGHTS + NUM_POINT_LIGHTS + NUM_SPOT_LIGHTS; ++index)
    {
        result += ComputeSpotLight(lights[index], material, illuminatedPosition, normal, toEyeVector);
    }
#endif 

    return float4(result, 0.0f);
}


