#include "Light.hlsl"

// ==== POM 辅助函数 ====
// 构建正交化的 TBN，确保 T 与 N 正交
float3x3 BuildTBN(float3 N, float3 T)
{
    T = normalize(T - N * dot(N, T));
    float3 B = normalize(cross(N, T));
    return float3x3(T, B, normalize(N));
}

// Parallax Occlusion Mapping 主算法
// 大掠射角时边缘会有分层现象 用UE类似的TAA方法可缓解 暂时不实现
float2 ParallaxOcclusionMapping(Texture2D heightMap, SamplerState smp, float2 uv, float3 viewDirTS, float heightScale, int minLayers, int maxLayers)
{
    // 视角相关的自适应层数
    float ndotv = saturate(viewDirTS.z);
    int layers = (int)lerp((float)maxLayers, (float)minLayers, ndotv);

    float layerDepth = 1.0f / layers;
    float currentLayerDepth = 0.0f;

    // 视线与高度缩放换算的步进（切空间）
    float2 deltaUV = (viewDirTS.xy / max(viewDirTS.z, 0.001f)) * heightScale / layers;

    float2 curUV = uv;
    float curHeight = heightMap.Sample(smp, curUV).r;

    // 分层搜索
    [loop]
    for (int i = 0; i < layers && currentLayerDepth < curHeight; ++i)
    {
        curUV -= deltaUV;
        curHeight = heightMap.Sample(smp, curUV).r;
        currentLayerDepth += layerDepth;
    }

    // 线性插值提升精度（与上一层交点插值）
    float2 prevUV = curUV + deltaUV;
    float prevHeight = heightMap.Sample(smp, prevUV).r;

    float after = curHeight - currentLayerDepth;
    float before = prevHeight - (currentLayerDepth - layerDepth);
    float weight = saturate(before / (before - after + 1e-5f));

    return lerp(curUV, prevUV, weight);
}

float3 ACESFilm(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x*(a*x+b))/(x*(c*x+d)+e));
}

float3 GammaCorrection(float3 color)
{
    return pow(color, 0.4545f);
}

float4 PS(VertexOut pin) : SV_Target
{
    float3 toEyeW = gEyePosW - pin.posW;
    float disToEye = length(toEyeW);
    toEyeW /= disToEye; // 复用 disToEye
    
    MaterialData material = materialDataBuffer[pin.materialIndex];
    
    if(pin.textureFlags & MAT_USE_DEPTH_MAP)
    {
        Texture2D heightMap = textures[textureTableBuffer[pin.textureTableIndex].depthIndex];

        // TBN 与切空间视线
        float3x3 TBN = BuildTBN(pin.normalW, pin.tangentW);
        float3 viewDirTS = mul(transpose(TBN), toEyeW);

        // 参数：可按需求调优/做成材质参数
        const float heightScale = 0.025f;    // 浮雕强度
        const int   minLayers  = 8;
        const int   maxLayers  = 64;

        // 仅在正向视角下启用（避免背面发散）
        if (viewDirTS.z > 0.01f)
        {
            pin.texCoord = ParallaxOcclusionMapping(heightMap, gsamAnisotropicWrap, pin.texCoord, normalize(viewDirTS), heightScale, minLayers, maxLayers);
            pin.texCoord = saturate(pin.texCoord);
        }
    }

    if(pin.textureFlags & MAT_USE_ALBEDO_MAP)
    {
        material.albedo *= textures[textureTableBuffer[pin.textureTableIndex].diffuseIndex].Sample(gsamAnisotropicWrap, pin.texCoord);
    }
    
    if(pin.textureFlags & MAT_USE_NORMAL_MAP)
    {
        // 优化的 TBN 变换
        float3 bitangent = cross(pin.normalW, pin.tangentW);
        float3 normalTex = textures[textureTableBuffer[pin.textureTableIndex].normalIndex].Sample(gsamAnisotropicWrap, pin.texCoord).rgb;
        normalTex = normalTex * 2.0f - 1.0f;
        
        // 直接展开矩阵乘法
        pin.normalW = normalize(
            normalTex.x * pin.tangentW + 
            normalTex.y * bitangent + 
            normalTex.z * pin.normalW
        );
    }

    // 光照计算
    float4 ambientLight = gAmbientIlluminating * material.albedo;
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeAllLights(gLights, material, pin.posW, pin.normalW, toEyeW, shadowFactor);

    float4 allLightColor = ambientLight + directLight;

    // 后处理
    allLightColor.rgb = ACESFilm(allLightColor.rgb);
    allLightColor.rgb = GammaCorrection(allLightColor.rgb);
    allLightColor.a = material.albedo.a;

    return allLightColor;
}
