#include <Common/Intrinsics.slang> // required for the SV_DrawIndex

struct VertexInput {
    float2 position : POSITION;
    float3 color    : COLOR0;
};

struct VertexOutput {
    float4 position : SV_Position;
    float3 color    : COLOR0;
};

VertexOutput vertexMain(VertexInput input, uint vertexID : SV_VertexID, 
	uint drawID : SV_DrawIndex
)
 {
	float2 offsets[3] = {
		float2(0.0, 0.0),
		float2(0.5, 0.0),
		float2(-0.3, 0.3)
	};

    VertexOutput output;
    output.position = float4(input.position * 0.3 + offsets[drawID], 0.0, 1.0);
    output.color = input.color;
	
    return output;
}

float4 fragmentMain(VertexOutput input) : SV_Target
{
    return float4(input.color, 1.0);
}
