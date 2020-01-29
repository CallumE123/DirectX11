//BUMP PIXEL SHADER
Texture2D txStoneColor : register(t0);
Texture2D txStoneBump : register(t1);
SamplerState txStoneSampler : register(s0);

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
};

float4 main(VS_OUTPUT input) : SV_Target
{
	float4 stoneNormal = txStoneBump.Sample(txStoneSampler, input.TexCoord);
	float4 stoneCol = txStoneColor.Sample(txStoneSampler, input.TexCoord);
	float3 N = normalize(2.0* stoneNormal.xyz - 1.0);

	float4 lightCol = float4(1.0, 1.0, 1.0, 1.0);
	float4 intensity = 0.35;
	float power = 3;

	float3 lightDir = normalize(lightPos.xyz - input.Pos.xyz);

	float diff = max(0.0f, dot(lightDir, N));

	float4 diffColor = diff * lightCol;

	float3 V = normalize(Eye - input.Pos);
	float3 R = reflect(normalize(lightDir), N);

	float4 colour = stoneCol + (intensity * lightCol * pow(dot(R, V), power));

	return float4(colour.xyz, 1.0f);
}