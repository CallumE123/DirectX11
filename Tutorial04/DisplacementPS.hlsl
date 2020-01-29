Texture2D txDisp : register(t0);
SamplerState txDispSampler : register(s0);

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
};

float4 main(VS_OUTPUT input) : SV_TARGET
{
	return txDisp.Sample(txDispSampler, input.TexCoord).rrrr;
}