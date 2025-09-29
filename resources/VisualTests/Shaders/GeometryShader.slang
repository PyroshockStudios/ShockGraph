struct VSOutput {
    float4 position : SV_Position;
    float3 color    : COLOR0;
};

// Vertex shader - simple pass-through
VSOutput vertexMain(uint vertexID : SV_VertexID)
{
    float2 positions[3] = {
        float2( 0.0,  0.5),
        float2(-0.5, -0.5),
        float2( 0.5, -0.5)
    };
    float3 colors[3] = {
        float3(1, 0, 0),
        float3(0, 1, 0),
        float3(0, 0, 1)
    };

    VSOutput o;
    o.position = float4(positions[vertexID], 0.0, 1.0);
    o.color = colors[vertexID];
    return o;
}

// Geometry shader: expands one triangle into 6 triangles in a fan
[maxvertexcount(18)] // 6 triangles × 3 vertices each
void geometryMain(triangle VSOutput input[3], inout TriangleStream<VSOutput> triStream)
{
    // Emit original triangle
    triStream.Append(input[0]);
    triStream.Append(input[1]);
    triStream.Append(input[2]);
    triStream.RestartStrip();

    // Expand around center
    float4 center = (input[0].position + input[1].position + input[2].position) / 3.0;

    for (uint i = 0; i < 3; ++i) {
		const float4 offset = float4(1.25, 1.25, 1.0, 1.0);
        VSOutput a = input[i];
		a.position *= offset;
        VSOutput b = input[(i + 1) % 3];
		b.position *= offset;

        VSOutput c; 
        c.position = (input[i].position + input[(i + 1) % 3].position) * offset * float4(1.5, 1.5, 1.0, 1.0);
        c.color = lerp(float3(1, 0, 0), float3(0,1,1), ((float)i) / 2.0);

        triStream.Append(a);
        triStream.Append(b);
        triStream.Append(c);
        triStream.RestartStrip();
    }
}

float4 fragmentMain(VSOutput input) : SV_Target
{
    return float4(input.color, 1.0);
}