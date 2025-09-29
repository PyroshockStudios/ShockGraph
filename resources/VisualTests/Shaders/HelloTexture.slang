#include <Common/PushConstant.slang>
#include <Common/DescriptorIndexing.slang>

struct VertexOutput {
    float4 position : SV_Position;
    float2 uv       : TEXCOORD0;
};

struct TexturePush {
    PyroDescriptor Image;	
    PyroDescriptor Sampler;	
};

PYRO_PUSH_CONSTANT(TexturePush, gPush);

VertexOutput vertexMain(uint vertexID : SV_VertexID)
{
    // 6 vertices for 2 triangles
    float2 positions[6] = {
        float2(-0.5f,  0.5f),  // Top-left
        float2(-0.5f, -0.5f),  // Bottom-left
        float2( 0.5f, -0.5f),  // Bottom-right

        float2(-0.5f,  0.5f),  // Top-left
        float2( 0.5f, -0.5f),  // Bottom-right
        float2( 0.5f,  0.5f)   // Top-right
    };

    float2 uvs[6] = {
        float2(0.0f, 0.0f),
        float2(0.0f, 1.0f),
        float2(1.0f, 1.0f),

        float2(0.0f, 0.0f),
        float2(1.0f, 1.0f),
        float2(1.0f, 0.0f)
    };

    VertexOutput output;
    output.position = float4(positions[vertexID], 0.0, 1.0);
    output.uv = uvs[vertexID];
    return output;
}

float4 fragmentMain(VertexOutput input) : SV_Target
{
    return PYRO_ACCESS(gPush.Image, gTexture2D).Sample(
        PYRO_ACCESS(gPush.Sampler, gSampler),
        input.uv
    );
}
