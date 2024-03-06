cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;//物体世界变换矩阵
}
cbuffer cbPass : register(b1)
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
}
struct VertexIn
{
	float3 Pos	 :POSITION;
	float4 Color :COLOR;
};
struct VertexOut
{
    float4 PosH : SV_Position;
    float4 Color : COLOR;
};

VertexOut VS(VertexIn vin)
{   
    //vin.Pos.xy += 0.5f*sin(vin.Pos.x)*sin(3.0f * gTimer);
    //vin.Pos.xz *= 0.1f;
    //vin.Pos.y += sin(10.0f * gTimer);
    VertexOut vout;
    
    float4 PosW = mul(float4(vin.Pos, 1.0f), gWorld);
    vout.PosH = mul(PosW, gViewProj);
    vout.Color = vin.Color;
    
    return vout;
}

float4 PS( VertexOut pin ) : SV_Target
{
    //pin.Color.r *= sin(gTimer);
    
	return pin.Color;
}