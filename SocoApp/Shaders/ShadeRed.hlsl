Texture2D gInput            : register(t0);
RWTexture2D<float4> gOutput : register(u0);

[numthreads(32, 32, 1)]
void ShadeRed(int3 groupThreadID : SV_GroupThreadID,
				int3 dispatchThreadID : SV_DispatchThreadID)
{
	gOutput[dispatchThreadID.xy] = gInput[dispatchThreadID.xy] * float4(1, 0, 1, 1);
}