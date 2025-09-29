#include <Common/PushConstant.slang>

struct VertexOutput {
    float4 position : SV_Position;
};

// Vertex shader
VertexOutput vertexMain(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    float2 positions[3] = {
        float2( 0.0f,  0.25f),
        float2(-0.35f, -0.35f),
        float2( 0.35f, -0.35f)
    };

    VertexOutput output;
    output.position = float4(positions[vertexID] + ((float)instanceID) * 0.04, 0.0, 1.0);

    return output;
}

// Fragment shader
float4 fragmentMain(VertexOutput input) : SV_Target
{
    return float4(1.0, 1.0, 1.0, 0.1);
}
