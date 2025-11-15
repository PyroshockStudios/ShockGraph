#include <Common/UnorderedAccessView.slang>
#include <Common/Intrinsics.slang>
#include <Common/PushConstant.slang>
#include <Common/DescriptorIndexing.slang>

PYRO_BIND_UNORDERED_ACCESS_TEXTURE_2D(0, float4, gOutput);
PYRO_PUSH_CONSTANT(PyroDescriptor, tlasIndex);

[numthreads(8,8,1)]
void computeMain(uint3 dispatchThreadID : SV_DispatchThreadID) {

    const float ZOOM = 0.2;
    int width, height;
    gOutput.GetDimensions(width, height);

    RayDesc ray;
    float2 uv = (dispatchThreadID.xy + 0.5) / float2(width, height);
	float2 ndc = uv * 2.0 - 1.0;
    float3 origin = float3(ndc / ZOOM, 0.0);
    float3 direction = float3(0.0, 0.0, 1.0);
    ray.Origin = origin;
    ray.Direction = direction;
    ray.TMin = 0.1;
    ray.TMax = 100.0;

    RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_FORCE_OPAQUE> query;
	query.TraceRayInline(
        gTlas[tlasIndex],
        RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_FORCE_OPAQUE,
        0xFF,
        ray
    );

    uint hit = 0;
	query.Proceed();

	if (query.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
	{
		hit = 1; 
	}
	else
	{
		hit = 0; // No committed hit found
	}
	
    gOutput[dispatchThreadID.xy] = hit == 1 ? float4(1.0, 0.0, 0.0, 1.0) : float4(0.0, 0.0, 0.0, 1.0); // float4(origin, 1.0);
}
