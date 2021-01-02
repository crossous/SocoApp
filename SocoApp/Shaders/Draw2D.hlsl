#include "Common.hlsli"

Texture2D MainTex : register(t0);

cbuffer cbTexPosition : register(b0)
{
    float2 gTexSize;
    float2 gTexOffset;
}

struct VertexOut
{
    float4 PositionCS : SV_POSITION;
    float2 uv : TEXCOORD;
};

VertexOut VS(uint vI : SV_VERTEXID)
{
    VertexOut o = (VertexOut)0;
	o.uv = float2(vI & 1,vI >> 1);
    float2 pos = float2((o.uv.x - 0.5f) * 2,-(o.uv.y - 0.5f) * 2);
    pos = (pos + 1) * 0.5f;
    pos = pos * gTexSize + gTexOffset;
    pos = pos * 2 - 1;
    o.PositionCS = float4(pos,0,1);
	return o;
}

float4 PS(VertexOut i) : SV_TARGET
{
	return MainTex.Sample(gsamAnisotropicWrap, i.uv);
}