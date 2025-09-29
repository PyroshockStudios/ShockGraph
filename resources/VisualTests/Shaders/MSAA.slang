#include <Common/PushConstant.slang>

struct VertexOutput {
    float4 position : SV_Position;
    float3 color    : COLOR0;
};

// Vertex shader
VertexOutput vertexMain(uint vertexID : SV_VertexID)
{
    float theta = 20.0 * 3.14159265 / 180.0; // 20 degrees in radians
    float cosTheta = cos(theta);
    float sinTheta = sin(theta);

    float2 positions[3] = {
        float2( 0.0f,  0.8f),
        float2(-0.8f, -0.8f),
        float2( 0.8f, -0.8f)
    };

    // Apply 2D rotation
    for(int i = 0; i < 3; i++)
    {
        float2 p = positions[i];
        positions[i] = float2(
            p.x * cosTheta - p.y * sinTheta,
            p.x * sinTheta + p.y * cosTheta
        );
        positions[i] += float2(-0.15, 0.1);
    }

    VertexOutput output;
    output.position = float4(positions[vertexID], 0.0, 1.0);

    if(vertexID == 0) output.color = float3(0, 1, 1);
    else if(vertexID == 1) output.color = float3(1, 0, 1);
    else output.color = float3(1, 1, 0);

    return output;
}

// Fragment shader
float4 fragmentMain(VertexOutput input) : SV_Target
{
    return float4(input.color, 1.0);
}
