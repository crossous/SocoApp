#include "Common.hlsli"

Texture2D EarthDayMap : register(t0);
Texture2D EarthNightMap : register(t1);
Texture2D EarthCloudMap : register(t2);

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
    float4 dayAlbedo = EarthDayMap.Sample(gsamAnisotropicWrap, pin.TexC);
    float4 nightAlbedo = EarthNightMap.Sample(gsamAnisotropicWrap, pin.TexC);
    float4 cloudAlbedo = EarthCloudMap.Sample(gsamAnisotropicWrap, pin.TexC);

    Light mainLight = gLights[1];
    float3 lightDir = mainLight.Position - pin.PositionWS.xyz;

    float ndotl = saturate(dot(lightDir, pin.NormalWS));

    float3 color = lerp(dayAlbedo.xyz, cloudAlbedo.xxx, cloudAlbedo.x);
    color = lerp(nightAlbedo.xyz, color, ndotl);
    return float4(color, 1);
}