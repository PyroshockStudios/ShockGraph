#include <Common/PushConstant.slang>


PYRO_PUSH_CONSTANT(float, gPushRotation);

struct VertexOutput {
    float4 position : SV_Position;
    float3 color    : COLOR0;
};

// Vertex shader
VertexOutput vertexMain(uint vertexID : SV_VertexID)
{
    float2 positions[3] = {
        float2( 0.0f,  0.5f),
        float2(-0.5f, -0.5f),
        float2( 0.5f, -0.5f)
    };

    float3 colors[3] = {
        float3(1.0, 0.0, 0.0), // Red
        float3(0.0, 1.0, 0.0), // Green
        float3(0.0, 0.0, 1.0)  // Blue
    };
	
    float2x2 rotation = float2x2(
        cos(gPushRotation), -sin(gPushRotation),
        sin(gPushRotation),  cos(gPushRotation)
    );
	
    VertexOutput output;
    output.position = float4(mul(rotation, positions[vertexID]), 0.0, 1.0);
    output.color = colors[vertexID];
    return output;
}

// Fragment shader　
float4 fragmentMain(VertexOutput input) : SV_Target
{
    return float4(input.color, 1.0);
}