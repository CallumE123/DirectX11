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
	output.Pos = mul(input.Pos, World);
	output.WorldPos = output.Pos;

	float fRepeat = 10.0f;
	output.TexCoord = input.TexCoord * fRepeat;

	float dBias = 1.0f;
	float dScale = 5.0f;

	float displacement = txDisp.SampleLevel(txDispSampler, output.TexCoord, 0).r;
	displacement = displacement * dScale + dBias;

	float3 dPos = output.WorldPos.xyz + input.Normal * displacement;

	output.Pos = mul(float4(dPos, 1), View);
	output.Pos = mul(output.Pos, Projection);

	//Normalise
	output.Normal = mul(input.Normal, (float3x3)World);
	output.Normal = normalize(output.Normal);
	output.Tangent = input.Tangent;
	output.Binormal = input.Binormal;

	return output;
}