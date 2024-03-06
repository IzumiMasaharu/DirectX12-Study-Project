cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;//��������任����
}
cbuffer cbPass : register(b1)
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