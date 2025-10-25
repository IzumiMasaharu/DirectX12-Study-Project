struct MaterialData
{
	float4 diffuseAlbedo;			// 漫反射反照率
	float3 fresneRf0;				// 菲涅尔效应材质属性Rf（0°）
	float roughness;				// 材质粗糙度
	float3 emissive;				// 自发光
	float metallic;					// 金属度
	float4x4 materialTransform;
};