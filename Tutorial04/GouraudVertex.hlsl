//MAIN CUBE VERTEX
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
	float4 Colour : POSITION1;
	float3 viewDir: POSITION2;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output = (VS_OUTPUT)0;

	float4 ambient = float4(0.1, 0.2, 0.2, 1.0);
	float4 diffuse = float4(0.9, 0.7, 1.0, 1.0);
	float spec = 0.2f;

	float3 lightDir = normalize(lightPos.xyz - input.Pos.xyz);

	float diffLighting = saturate(dot(lightDir, input.Normal));

	diffLighting *= ((length(lightDir) * length(lightDir)) / dot(lightPos - input.Pos, lightPos - input.Pos));

	float3 H = normalize(normalize(Eye - input.Pos) - lightDir);
	float specular = pow(saturate(dot(H, input.Normal)), 2.0f);

	float4 colour = saturate(ambient + (diffuse * diffLighting * 0.6f) + (specular * 0.5f));

	colour = colour * normalize(lightCol);

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
	output.Colour = colour;
	output.viewDir = input.Pos.xyz;

	return output;
}