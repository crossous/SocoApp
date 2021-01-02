#include "Common.hlsli"

Texture2D SunNoiseMap : register(t0);

cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
};

struct VertexIn
{
	float3 PositionOS    : POSITION;
    float3 NormalOS : NORMAL;
	float2 TexC    : TEXCOORD;
};

struct VertexOut
{
	float4 PositionCS    : SV_POSITION;
	float2 TexC    : TEXCOORD;
    float4 PositionWS    : TEXCOORD1;
    float3 NormalWS : TEXCOORD2;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

	vout.PositionWS = mul(float4(vin.PositionOS, 1), gWorld);
    vout.PositionCS = mul(vout.PositionWS, gViewProj);

    vout.NormalWS = mul(vin.NormalOS, (float3x3)gWorld);
    vout.TexC = vin.TexC;

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    // Light mainLight = gLights[0];
    // float ndotl = saturate(dot(-mainLight.Direction, pin.NormalWS.xyz));
    // return float4(ndotl.xxx, 1);
    float4 sunAlbedo = SunNoiseMap.Sample(gsamAnisotropicWrap, pin.TexC);

    float3 color = lerp(float3(0.603, 0.07, 0.07), float3(1, 0.7, 0.227), sunAlbedo.r);

    return float4(color, 1);
}