struct VertexOutput {
    float4 position : SV_Position;
    float3 color    : COLOR0;
};

VertexOutput vertexMain(uint vertexID : SV_VertexID)
{
    uint quadIndex = vertexID / 6;
    uint vertexInQuad = vertexID % 6;

    float2 quadVerts[6] = {
        float2(0, 0), float2(1, 0), float2(0, 1),
        float2(1, 0), float2(1, 1), float2(0, 1)
    };

    int gridX = quadIndex % 3;
    int gridY = quadIndex / 3;

    float2 quadSize = float2(0.3, 0.3);   
    float2 spacing = float2(0.35, 0.35);  
    float2 origin = float2(-0.5 + gridX * spacing.x, -0.5 + gridY * spacing.y);

    float2 pos = origin + quadVerts[vertexInQuad] * quadSize;

    VertexOutput output;
    output.position = float4(pos, 0.0, 1.0);
    output.color = float3(1, 1, 1); 
    return output;
}

float4 fragmentMain(VertexOutput input) : SV_Target
{
    return float4(input.color, 1.0);
}
