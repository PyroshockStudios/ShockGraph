#include <Common/UniformBufferView.slang>

struct VertexColors {
	float4 colors[4];
};

PYRO_BIND_UNIFORM_BUFFER(0, VertexColors, gUbo);

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

	
    VertexOutput output;
    output.position = float4(positions[vertexID], 0.0, 1.0);
    output.color = gUbo.colors[vertexID].xyz;
    return output;
}

// Fragment shader　
float4 fragmentMain(VertexOutput input) : SV_Target
{
    return float4(input.color, 1.0);
}