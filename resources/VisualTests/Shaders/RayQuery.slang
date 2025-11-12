#include <Common/UnorderedAccessView.slang>
#include <Common/Intrinsics.slang>

// Bind an array of TLAS handles at t0 (space1). The runtime maps created TLAS into this array.
[[vk::binding(3, 0)]] RaytracingAccelerationStructure gTlas[] : register(t0, space1);
PYRO_BIND_UNORDERED_ACCESS_TEXTURE_2D(0, float4, gOutput);

[[push_constant]] cbuffer pyro_PushConstant : register(b13, space0) { uint tlasIndex; };

[numthreads(8,8,1)]
void computeMain(uint3 dispatchThreadID : SV_DispatchThreadID) {
    RayQuery<RAY_FLAG_NONE> query;

    int width, height;
    gOutput.GetDimensions(width, height);

    RayDesc ray;
    float2 uv = (dispatchThreadID.xy + 0.5) / float2(width, height);
    float3 origin = float3(uv * 2.0 - 1.0, -1.0);
    float3 direction = float3(0.0, 0.0, 1.0);
    ray.Origin = origin;
    ray.Direction = direction;
    ray.TMin = 0.001;
    ray.TMax = 1000.0;

    query.TraceRayInline(
        gTlas[tlasIndex],
        RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,
        0xFF,
        ray
    );

    uint hit = 0;
    if (query.CommittedStatus() == COMMITTED_TRIANGLE_HIT) {
        hit = 1;
    }
    gOutput[dispatchThreadID.xy] = hit == 1 ? float4(1.0, 0.0, 0.0, 1.0) : float4(0.0, 0.0, 0.0, 1.0);
}
