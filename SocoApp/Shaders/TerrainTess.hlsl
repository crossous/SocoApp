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
	float3 PositionOS    : POSITION;
	float2 TexC    : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

    vout.PositionOS = vin.PositionOS;
    vout.TexC = vin.TexC;

    // int3 SamplePosition = int3(vout.TexC * float2(HeightMapWidth - 1, HeightMapHeight - 1), 0);
    // float height = HeightMap.Load(SamplePosition).x;
    // vout.PositionOS.y = height * Height;


    return vout;
}

#define TRIANGLE_VERTEX_NUM 4

struct PatchTess
{
	float EdgeTess[TRIANGLE_VERTEX_NUM]   : SV_TessFactor;
	float InsideTess[2] : SV_InsideTessFactor;
};

float CalcQuadTessFactor(float3 point1, float3 point2)
{
    const float maxDist = 512;
    const float minDist = 16;
    const float maxFrac = 16;
    const float minFrac = 1;

    float3 middlePoint = (point1 + point2) * 0.5;
    float dist = distance(middlePoint, gEyePosW);

    //return pow(2, lerp(6, 0, saturate((dist - 16.0f)/(256.0f - 16.0f))));
    //return lerp(minFrac, maxFrac, (1 - saturate((dist - minDist) / (maxDist - minDist))));
    return pow(2, lerp(0, 4, (1 - saturate((dist - minDist) / (maxDist - minDist)))));
}

float CalcQuadTessFactor(float3 point1, float3 point2, float3 point3, float3 point4)
{
    const float maxDist = 512;
    const float minDist = 16;
    const float maxFrac = 16;
    const float minFrac = 1;

    float3 middlePoint = (point1 + point2 + point3 + point4) * 0.25;
    float dist = distance(middlePoint, gEyePosW);

    //return pow(2, lerp(6, 0, saturate((dist - 16.0f)/(256.0f - 16.0f))));
    return pow(2, lerp(0, 4, (1 - saturate((dist - minDist) / (maxDist - minDist)))));
}

PatchTess ConstantHS(InputPatch<VertexOut, TRIANGLE_VERTEX_NUM> patch, uint patchID : SV_PrimitiveID)
{
    PatchTess pt;

    float dist = distance(gEyePosW, patch[0].PositionOS);
    //float tessFactor = minFrac + (maxFrac - minFrac) * (1 - saturate((dist - minDist) / (maxDist - minDist)));
    float tessFactor = 16;

    // pt.EdgeTess[0] = tessFactor;
    // pt.EdgeTess[1] = tessFactor;
    // pt.EdgeTess[2] = tessFactor;
    // pt.EdgeTess[3] = tessFactor;
    // pt.InsideTess[0] = tessFactor;
    // pt.InsideTess[1] = tessFactor;
    pt.EdgeTess[0] = CalcQuadTessFactor(patch[0].PositionOS, patch[2].PositionOS);
    pt.EdgeTess[3] = CalcQuadTessFactor(patch[2].PositionOS, patch[3].PositionOS);
    pt.EdgeTess[2] = CalcQuadTessFactor(patch[3].PositionOS, patch[1].PositionOS);
    pt.EdgeTess[1] = CalcQuadTessFactor(patch[1].PositionOS, patch[0].PositionOS);
    pt.InsideTess[0] = CalcQuadTessFactor(patch[0].PositionOS, patch[1].PositionOS, patch[2].PositionOS, patch[3].PositionOS);
    pt.InsideTess[1] = pt.InsideTess[0];

    // pt.EdgeTess[0] = 4;
    // pt.EdgeTess[1] = 4;
    // pt.EdgeTess[2] = 4;
    // pt.EdgeTess[3] = 4;
    // pt.InsideTess[0] = 4;
    // pt.InsideTess[1] = 4;

    return pt;
}

struct HullOut
{
    float3 PositionOS : POSITION;
    float2 TexC : TEXCOORD;
};

[domain("quad")]
[partitioning("fractional_even")]
//[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(TRIANGLE_VERTEX_NUM)]
[patchconstantfunc("ConstantHS")]
//[maxtessfactor(64.0f)]
HullOut HS(InputPatch<VertexOut, TRIANGLE_VERTEX_NUM> p,
            uint i : SV_OutputControlPointID,
            uint patchId : SV_PrimitiveID)
{
    HullOut hout;
    hout.PositionOS = p[i].PositionOS;
    hout.TexC = p[i].TexC;
    return hout;
}

struct DomainOut
{
    float4 PositionCS    : SV_POSITION;
	float2 TexC    : TEXCOORD;
    float4 PositionWS    : TEXCOORD1;
    // float4 NormalWS : TEXCOORD2;
    float3 Color : TEXCOORD3;
};

float3 EstimateNormal(float2 texcoord)
{
    float2 uvOffset = float2(1.0 / HeightMapWidth, 1.0 / HeightMapHeight);
    float uvScale = 1;

    float2 uvb = texcoord + uvOffset * float2(0, -1) * uvScale;
    float2 uvc = texcoord + uvOffset * float2(1, -1) * uvScale;
    float2 uvd = texcoord + uvOffset * float2(1, 0) * uvScale;
    float2 uve = texcoord + uvOffset * float2(1, 1) * uvScale;
    float2 uvf = texcoord + uvOffset * float2(0, 1) * uvScale;
    float2 uvg = texcoord + uvOffset * float2(-1, 1) * uvScale;
    float2 uvh = texcoord + uvOffset * float2(-1, 0) * uvScale;
    float2 uvi = texcoord + uvOffset * float2(-1, -1) * uvScale;

    float yb = HeightMap.SampleLevel(gsamLinearClamp, uvb, 0).x * Height;
    float yc = HeightMap.SampleLevel(gsamLinearClamp, uvc, 0).x * Height;
    float yd = HeightMap.SampleLevel(gsamLinearClamp, uvd, 0).x * Height;
    float ye = HeightMap.SampleLevel(gsamLinearClamp, uve, 0).x * Height;
    float yf = HeightMap.SampleLevel(gsamLinearClamp, uvf, 0).x * Height;
    float yg = HeightMap.SampleLevel(gsamLinearClamp, uvg, 0).x * Height;
    float yh = HeightMap.SampleLevel(gsamLinearClamp, uvh, 0).x * Height;
    float yi = HeightMap.SampleLevel(gsamLinearClamp, uvi, 0).x * Height;

    float3 normal = float3(-yc-2*yd-ye+yg+2*yh+yi, 8, 2*yb+yc-ye-2*yf-yg+yi);

    return normalize(normal);
}


[domain("quad")]
DomainOut DS(PatchTess patchTess,
            float2 tessUV : SV_DomainLocation,
            const OutputPatch<HullOut, TRIANGLE_VERTEX_NUM> quad)
{
    DomainOut dout = (DomainOut)0;;

    //float3 PositionOS = tri[0].PositionOS * uvw.x + tri[1].PositionOS * uvw.y + tri[2].PositionOS * uvw.z;
    float3 PositionOS = lerp(lerp(quad[0].PositionOS, quad[1].PositionOS, tessUV.x), lerp(quad[2].PositionOS, quad[3].PositionOS, tessUV.x), tessUV.y);

    dout.PositionWS = float4(PositionOS, 1);

    //dout.TexC = tri[0].TexC * uvw.x + tri[1].TexC * uvw.y + tri[2].TexC * uvw.z;
    dout.TexC = lerp(lerp(quad[0].TexC, quad[1].TexC, tessUV.x), lerp(quad[2].TexC, quad[3].TexC, tessUV.x), tessUV.y);

    int3 SamplePosition = int3(dout.TexC * float2(HeightMapWidth - 1, HeightMapHeight - 1), 0);
    //float height = HeightMap.Load(SamplePosition).x;
    float height = HeightMap.SampleLevel(gsamLinearClamp, dout.TexC, 0).x;

    dout.PositionWS.y = height * Height;

    dout.PositionCS = mul(dout.PositionWS, gViewProj);

    //dout.NormalWS = float4(EstimateNormal(dout.TexC), 0);

    if(tessUV.x == 0 || tessUV.y == 0 || tessUV.x == 1 || tessUV.y == 1)
        dout.Color = float3(1, 0, 0);
    else
        dout.Color = float3(0, 1, 0);

    return dout;
}

float4 PS(DomainOut pin) : SV_Target
{
    //return float4(pin.Color, 1);
    //pin.NormalWS = normalize(pin.NormalWS);
    float4 Albedo = HeightMap.Sample(gsamAnisotropicWrap, pin.TexC);
    //return Albedo;
    Light mainLight = gLights[0];

    float3 normal = EstimateNormal(pin.TexC);

    float ndotl = saturate(dot(-normalize(mainLight.Direction), normal));

    return float4(0, ndotl.x, 0, 1);
}