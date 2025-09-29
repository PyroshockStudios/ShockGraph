#include <Common/UniformBufferView.slang>

struct VertexScale {
	float scale;
};

PYRO_BIND_UNIFORM_BUFFER(0, VertexScale, gUbo);

struct VertexOutput {
    float4 position : SV_Position;
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
    output.position = float4(positions[vertexID] * float2(1.0, gUbo.scale), 0.0, 1.0);
    return output;
}

// Fragment shader　
float4 fragmentMain(VertexOutput input) : SV_Target
{
    return float4(1.0, 1.0, 1.0, 1.0);
}