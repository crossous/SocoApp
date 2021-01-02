#include "Common.hlsli"

TextureCube gCubeMap : register(t0);

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
    float4 PositionWS : TEXCOORD1;
    float3 NormalWS : TEXCOORD2;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

    //vout.PositionCS = mul(float4(vin.PositionOS, 1), gViewProj);

    vout.PositionCS = mul(float4(mul(vin.PositionOS, (float3x3)gView), 1), gProj);

    vout.PositionCS = vout.PositionCS.xyww;
    vout.PositionWS = float4(vin.PositionOS, 1);
    vout.NormalWS = vin.NormalOS;
    vout.TexC = vin.TexC;

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    return gCubeMap.Sample(gsamLinearWrap, pin.PositionWS.xyz);
}