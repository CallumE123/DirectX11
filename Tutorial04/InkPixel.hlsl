cbuffer ConstantBuffer : register(b0)
{
	matrix World;
	matrix View;
	matrix Projection;
	float4 lightPos;
	float4 lightCol;
	float4 lightAmb;
	float4 lightDiff;
	float4 Eye;
}

struct VS_OUTPUT
{
	float4 Pos : SV_POSITION;
	float4 WorldPos : POSITION;
	float3 Normal : NORMAL;
	float2 TexCoord : TEXCOORD;
	float3 Tangent : TANGENT;
	float3 Binormal : BINORMAL;
	float3 viewDir: POSITION1;
};

float4 main(VS_OUTPUT input) : SV_Target
{
	float4 inkCol = float4(0.4f, 0.3f, 0.7f, 0.0f);

	return float4 (0.5 * inkCol.xyz, 0.6);
}