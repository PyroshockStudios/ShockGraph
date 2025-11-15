#include <Common/Intrinsics.slang>
#include <Common/PushConstant.slang>
#include <Common/DescriptorIndexing.slang>

PYRO_PUSH_CONSTANT(PyroDescriptor, tlasIndex);

struct VertexOutput {
    float4 position : SV_Position;
    float2 uv       : TEXCOORD0;
};

VertexOutput vertexMain(uint vertexID : SV_VertexID)
{
    // 6 vertices for 2 triangles
    float2 positions[6] = {
        float2(-1.0f,  1.0f),  // Top-left
        float2(-1.0f, -1.0f),  // Bottom-left
        float2( 1.0f, -1.0f),  // Bottom-right

        float2(-1.0f,  1.0f),  // Top-left
        float2( 1.0f, -1.0f),  // Bottom-right
        float2( 1.0f,  1.0f)   // Top-right
    };

    float2 uvs[6] = {
        float2(0.0f, 0.0f),
        float2(0.0f, 1.0f),
        float2(1.0f, 1.0f),

        float2(0.0f, 0.0f),
        float2(1.0f, 1.0f),
        float2(1.0f, 0.0f)
    };

    VertexOutput output;
    output.position = float4(positions[vertexID], 0.0, 1.0);
    output.uv = uvs[vertexID];
    return output;
}

float4 fragmentMain(VertexOutput input) : SV_Target
{
    
    const float ZOOM = 0.2;

	float2 ndc = input.uv * 2.0 - 1.0;
    float3 origin = float3(ndc / ZOOM, 0.0);
    float3 direction = float3(0.0, 0.0, 1.0);
    RayDesc ray;
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
	
    return hit == 1 ? float4(1.0, 0.0, 0.0, 1.0) : float4(0.0, 0.0, 0.0, 1.0); // float4(origin, 1.0);
}
