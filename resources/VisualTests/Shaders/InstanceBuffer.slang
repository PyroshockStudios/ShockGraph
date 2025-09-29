struct VertexInput {
    float2 position : POSITION;
    float3 color    : COLOR0;
    float2 offset   : TEXCOORD0; 
};

struct VertexOutput {
    float4 position : SV_Position;
    float3 color    : COLOR0;
};

VertexOutput vertexMain(VertexInput input, uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    VertexOutput output;
    // Apply per-instance offset
    output.position = float4(input.position + input.offset, 0.0, 1.0);
    output.color = input.color;
    return output;
}

float4 fragmentMain(VertexOutput input) : SV_Target
{
    return float4(input.color, 1.0);
}
