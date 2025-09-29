struct VertexOutput {
    float4 position : SV_Position;
    float3 color    : COLOR0;
};

// Vertex shader
VertexOutput vertexMain(uint vertexID : SV_VertexID)
{
    float2 positions[3] = {
        float2( 0.0f,  0.3f),
        float2(-0.3f, -0.3f),
        float2( 0.3f, -0.3f)
    };

    float3 colors[3] = {
        float3(1.0, 0.0, 0.0), // Red
        float3(0.0, 1.0, 0.0), // Green
        float3(0.0, 0.0, 1.0)  // Blue
    };

    VertexOutput output;
    output.position = float4(positions[vertexID], 0.0, 1.0);
    output.color = colors[vertexID];
    return output;
}

// Fragment shader　
float4 fragmentMain(VertexOutput input) : SV_Target
{
    return float4(input.color, 1.0);
}