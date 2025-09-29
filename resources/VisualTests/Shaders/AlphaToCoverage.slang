struct VertexOutput {
    float4 position : SV_Position;
    float3 color    : COLOR0;
    float2 uv       : TEXCOORD0;
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

    float2 uvs[3] = {
        float2(1.0, 0.0), 
        float2(0.0, 1.0), 
        float2(0.0, 0.0)  
    };


    VertexOutput output;
    output.position = float4(positions[vertexID], 0.0, 1.0);
    output.color = colors[vertexID];

    output.uv = uvs[vertexID];

    return output;
}

// Fragment shader
float4 fragmentMain(VertexOutput input) : SV_Target
{
    float alpha = saturate(input.uv.x + input.uv.y);
    return float4(input.color, alpha);
}
