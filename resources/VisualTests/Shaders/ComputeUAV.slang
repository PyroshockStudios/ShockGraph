#include <Common/UnorderedAccessView.slang>
#include <Common/PushConstant.slang>

struct VertexInput {
    float4 position : POSITION;
    float4 color    : COLOR0;
};

struct VertexOutput {
    float4 position : SV_Position;
    float4 color    : COLOR0;
};

PYRO_PUSH_CONSTANT(float, gTimeVtx);

VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;
    output.position = input.position;
    output.color = input.color;
    return output;
}

float4 fragmentMain(VertexOutput input) : SV_Target
{
    return float4(input.color.xyz, 1.0);
}

PYRO_BIND_UNORDERED_ACCESS_STRUCTURED_BUFFER(0, VertexInput, gVertexBufferUAV);
PYRO_BIND_UNORDERED_ACCESS_BYTE_ADDRESS_BUFFER(1, gIndexBufferUAV);
PYRO_PUSH_CONSTANT(float, gTime);

// Torus mesh parameters
static const uint GRID_SIZE_U = 64; // around the main ring
static const uint GRID_SIZE_V = 32; // around the tube

static const float R = 0.3; // major radius
static const float r = 0.1; // tube radius

// Color function (simple gradient along tube)
float4 ColorFunc(float u, float v)
{
    float t = (sin(v * 6.2831 + gTime) + 1.0) * 0.5;
    return lerp(float4(0,0,1,1), float4(1,0,0,1), t);
}

// Compute shader
[numthreads(8,8,1)]
void computeMain(uint3 id : SV_DispatchThreadID)
{
    uint uIdx = id.x;
    uint vIdx = id.y;

    if (uIdx >= GRID_SIZE_U || vIdx >= GRID_SIZE_V)
        return;

    uint vertexIndex = vIdx * GRID_SIZE_U + uIdx;

    float u = (float(uIdx) / GRID_SIZE_U) * 6.2831853; // 0..2pi
    float v = (float(vIdx) / GRID_SIZE_V) * 6.2831853;

    // Torus parametric equation
    float cosV = cos(v);
    float sinV = sin(v);
    float cosU = cos(u);
    float sinU = sin(u);

    float3 pos = float3(
        (R + r * cosV) * cosU,
        r * sinV,
        (R + r * cosV) * sinU
    );

    // Model rotation around Y axis
    float angle = gTime;
    float c = cos(angle);
    float s = sin(angle);
    float3 rotatedPos = float3(
        pos.x * c - pos.z * s,
        pos.y,
        pos.x * s + pos.z * c
    );
	
	float4x4 projViewModel = float4x4(
		1.2065, 0.0,     0.0,      0.0,
		0.0,    2.1445,  0.0,      0.0,
		0.0,   -0.3536,  1.00001,  1.0,
		0.0,    0.0,    -4.001,    0.0
	);

	float3 origin = float3(0.0, 0.5, 0.8);

    // Transform to clip space
    float4 clipPos = mul(projViewModel, float4(rotatedPos - origin, 1.0));
	
	clipPos.xy = clipPos.xy * 1.8 + float2(0.0, 0.95);

    VertexInput vert;
    vert.position = clipPos;
    vert.color = ColorFunc(u, v);

    gVertexBufferUAV[vertexIndex] = vert;

    // Write indices for triangles
    if (uIdx < GRID_SIZE_U - 1 && vIdx < GRID_SIZE_V - 1)
    {
        uint i0 = vertexIndex;
        uint i1 = vertexIndex + 1;
        uint i2 = vertexIndex + GRID_SIZE_U;
        uint i3 = i2 + 1;

        uint baseTri = (vIdx * (GRID_SIZE_U-1) + uIdx) * 6;
        uint byteOffset = baseTri * 4; // 4 bytes per uint

        gIndexBufferUAV.Store(byteOffset +  0, i0);
        gIndexBufferUAV.Store(byteOffset +  4, i2);
        gIndexBufferUAV.Store(byteOffset +  8, i1);

        gIndexBufferUAV.Store(byteOffset + 12, i1);
        gIndexBufferUAV.Store(byteOffset + 16, i2);
        gIndexBufferUAV.Store(byteOffset + 20, i3);
    }
}
