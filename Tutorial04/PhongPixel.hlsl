//LIGHTING PIXEL SHADER
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
	float3 lightDir = -normalize(lightPos.xyz - input.WorldPos.xyz);

	float diffLighting = saturate(dot(input.Normal, -lightDir));

	diffLighting *= ((length(lightDir) * length(lightDir)) / dot(lightPos - input.WorldPos, lightPos - input.WorldPos));

	float3 H = normalize(normalize(Eye - input.WorldPos) - lightDir);
	float specular = pow(saturate(dot(H, input.Normal)), 2.0f);

	float4 colour = saturate(lightAmb + (lightDiff * diffLighting) + specular);

	colour = colour * normalize(lightCol);
	return colour;
}