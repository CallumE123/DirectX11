//SPHERE 1 VERTEX
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

struct VS_INPUT {
	float4 Pos : POSITION;
	float3 Normal : NORMAL;
	float2 TexCoord : TEXCOORD;
	float3 Tangent : TANGENT;
	float3 Binormal : BINORMAL;
};

struct VS_OUTPUT
{
	float4 Pos : SV_POSITION;
	float4 WorldPos : POSITION;
	float3 Normal : NORMAL;
	float2 TexCoord : TEXCOORD;
	float3 Tangent : TANGENT;
	float3 Binormal : BINORMAL;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output = (VS_OUTPUT)0;

	//Apply Perspective to vertices
	output.Pos = mul(input.Pos, World);
	output.WorldPos = output.Pos;
	output.Pos = mul(output.Pos, View);
	output.Pos = mul(output.Pos, Projection);

	//Normalise
	output.Normal = mul(input.Normal, (float3x3)World);
	output.Normal = normalize(output.Normal);

	output.TexCoord = input.TexCoord;
	output.Tangent = input.Tangent;
	output.Binormal = input.Binormal;

	return output;
}