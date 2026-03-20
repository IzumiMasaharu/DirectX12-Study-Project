#ifndef MAX_BINDING_TEXTURE
#define MAX_BINDING_TEXTURE 8
#endif

#ifndef NUM_DIRECTIONAL_LIGHTS
#define NUM_DIRECTIONAL_LIGHTS 1
#endif
#ifndef NUM_POINT_LIGHTS
#define NUM_POINT_LIGHTS 1
#endif
#ifndef NUM_SPOT_LIGHTS
#define NUM_SPOT_LIGHTS 0
#endif

struct Light
{
    float3 rgbIntensity;
    float start;
    float3 direction;
    float end;
    float3 position;
    float spotPower;
};

static const uint MAT_USE_NO_MAP      = 0;
static const uint MAT_USE_ALBEDO_MAP  = 1 << 0;
static const uint MAT_USE_NORMAL_MAP  = 1 << 1;
static const uint MAT_USE_DEPTH_MAP   = 1 << 2;

SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

struct MaterialData
{
    float4  	albedo;
    float3  	fresnel;
    float   	roughness;
    float3  	emissive;
    float   	renderLayer;
    float       textureTableIndex;
};
StructuredBuffer<MaterialData> materialDataPool : register(t0, space1);

Texture2D textures[] : register(t0, space0);
struct TextureTable
{
    uint diffuseIndex;
    uint normalIndex;
    uint depthIndex;
    uint padding0;
};
StructuredBuffer<TextureTable> textureTablePool : register(t1, space1);

struct InstanceData
{
    float4x4 worldTransform;

    uint materialIndex;
    uint padding0;
    uint padding1;
    uint padding2;
};
StructuredBuffer<InstanceData> instanceDataBuffer : register(t2, space1);

cbuffer cbPass : register(b0)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;

    float3 gEyePosW;
    float  cbPerObjectPad1;

    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;

    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;

    float4 gAmbientIlluminating;

    Light gLights[MAX_NUM_LIGHTS];
}


struct VertexIn
{
    float3 pos      : POSITION;
    float3 normal   : NORMAL;
    float3 tangent  : TANGENT;
    float2 texCoord : TEXCOORD;
};

struct VertexOut
{
    float4 posH      : SV_POSITION;
    float3 posW      : POSITION;
    float3 normalW   : NORMAL;
    float3 tangentW  : TANGENT;
    float2 texCoord  : TEXCOORD;

    nointerpolation uint materialIndex : MATINDEX;
};
