#include <Common/SpecializationConstant.slang>

struct VertexOutput {
    float4 position : SV_Position;
    float3 color    : COLOR0;
};

PYRO_SPECIALIZATION(0, float, gBrightness, 0.0);
PYRO_SPECIALIZATION(1, float, gPositionX, 0.0);
PYRO_SPECIALIZATION(2, float, gPositionY, 0.2); // default value

// Vertex shader
VertexOutput vertexMain(uint vertexID : SV_VertexID)
{
    float2 positions[3] = {
        float2( 0.0f,  0.25f),
        float2(-0.25f, -0.25f),
        float2( 0.25f, -0.25f)
    };

    VertexOutput output;
    output.position = float4(positions[vertexID] + float2(gPositionX, gPositionY), 0.0, 1.0);
    output.color = float3(gBrightness);

    return output;
}

// Fragment shader
float4 fragmentMain(VertexOutput input) : SV_Target
{
    return float4(input.color, 1.0);
}
