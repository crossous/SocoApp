#include "Common.hlsli"

Texture2D<float4> HeightMap : register(t0);

cbuffer cbPerObject : register(b0)
{
    //float4x4 gWorld;
    float HeightMapWidth;
    float HeightMapHeight;
    float Height;
};

struct VertexIn
{
	float3 PositionOS    : POSITION;
	float2 TexC    : TEXCOORD;
};

struct VertexOut
{
	float4 PositionCS    : SV_POSITION;
	float2 TexC    : TEXCOORD;
    float4 PositionWS    : TEXCOORD1;
    float4 NormalWS : TEXCOORD2;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

    int3 SamplePosition = int3(vin.TexC * float2(HeightMapWidth - 1, HeightMapHeight - 1), 0);
    float height = HeightMap.Load(SamplePosition).x;
    vin.PositionOS.y = height * Height;

	//vout.PositionWS = mul(float4(vin.PositionOS, 1), gWorld);
    vout.PositionWS = float4(vin.PositionOS, 1);
    vout.PositionCS = mul(vout.PositionWS, gViewProj);
    vout.TexC = vin.TexC;

    // float yb = (HeightMap.Load(SamplePosition + int3(0, -1, 0)).x - height) * Height;
    // float yc = (HeightMap.Load(SamplePosition + int3(1, 0, 0)).x - height) * Height;
    // float yd = (HeightMap.Load(SamplePosition + int3(0, 1, 0)).x - height) * Height;
    // float ye = (HeightMap.Load(SamplePosition + int3(-1, 0, 0)).x - height) * Height;

    // float3 pointb = float3(0, yb, -1);
    // float3 pointc = float3(1, yc, 0);
    // float3 pointd = float3(0, yd, 1);
    // float3 pointe = float3(-1, ye, 0);

    // float3 faceNormalABC = cross(pointc, pointb);
    // float3 faceNormalACD = cross(pointd, pointc);
    // float3 faceNormalADE = cross(pointe, pointd);
    // float3 faceNormalAEB = cross(pointb, pointe);

    // vout.NormalWS = float4(normalize((faceNormalABC + faceNormalACD + faceNormalADE + faceNormalAEB) * 0.25), 0);
    // vout.NormalWS = float4((ye - yc)*0.5, 1, (yb - yd)*0.5, 0);

    int3 minUV = int3(0, 0, 0);
    int3 maxUV = int3(HeightMapWidth - 1, HeightMapHeight - 1, 0);

    int3 uvb = clamp(SamplePosition + int3(0, -1, 0), minUV, maxUV);
    int3 uvc = clamp(SamplePosition + int3(1, 0, 0), minUV, maxUV);
    int3 uvd = clamp(SamplePosition + int3(1, 1, 0), minUV, maxUV);
    int3 uve = clamp(SamplePosition + int3(0, 1, 0), minUV, maxUV);
    int3 uvf = clamp(SamplePosition + int3(-1, 0, 0), minUV, maxUV);
    int3 uvg = clamp(SamplePosition + int3(-1, -1, 0), minUV, maxUV);

    float yb = (HeightMap.Load(uvb).x - height) * Height;
    float yc = (HeightMap.Load(uvc).x - height) * Height;
    float yd = (HeightMap.Load(uvd).x - height) * Height;
    float ye = (HeightMap.Load(uve).x - height) * Height;
    float yf = (HeightMap.Load(uvf).x - height) * Height;
    float yg = (HeightMap.Load(uvg).x - height) * Height;

    float3 normal = float3(-yb-2*yc-yd+ye+2*yf+yg, 6, 2*yb+yc-yd-2*ye-yf+yg) / 6;
    vout.NormalWS = float4(normal, 0);
    vout.NormalWS = normalize(vout.NormalWS);

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    pin.NormalWS = normalize(pin.NormalWS);
    float4 Albedo = HeightMap.Sample(gsamAnisotropicWrap, pin.TexC);
    //return Albedo;
    Light mainLight = gLights[0];
    float ndotl = saturate(dot(-mainLight.Direction, pin.NormalWS.xyz));

    return float4(0, ndotl.x, 0, 1);
}