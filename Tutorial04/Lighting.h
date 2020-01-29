#pragma once
#include <DirectXMath.h>

using namespace DirectX;

struct Lighting
{
	XMFLOAT4 LightCol;
	XMFLOAT4 LightDiffuse;
	XMFLOAT4 LightAmbient;
	XMFLOAT4 LightPos;
};