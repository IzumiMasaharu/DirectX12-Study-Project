// 输入顶点数据
struct VertexIn
{
	float3 pos      : POSITION;
	float3 normal   : NORMAL;
	float3 tangentU : TANGENT;
	float2 texCoord : TEXCOORD;
};
// 输出顶点数据
struct VertexOut
{
	float4 posH     : SV_POSITION;	// 屏幕空间坐标
	float3 posW     : POSITION;		// 世界空间坐标
	float3 normalW  : NORMAL;
	float3 tangentW : TANGENT;
	float2 texCoord : TEXCOORD;
};