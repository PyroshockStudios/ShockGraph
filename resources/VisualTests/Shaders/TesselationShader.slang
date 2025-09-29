// Full tessellation chain: VS -> HS -> PatchConstants -> DS -> PS

struct VSOut {
    float3 position : POSITION;
    float3 color    : COLOR0;
};

VSOut vertexMain(uint vertexID : SV_VertexID)
{
    // Single triangular patch control points (you can change these or feed from a buffer)
    float3 positions[3] = {
        float3( 0.0f,  0.5f, 0.0f ),
        float3(-0.5f, -0.5f, 0.0f ),
        float3( 0.5f, -0.5f, 0.0f )
    };

    float3 colors[3] = {
        float3(1.0f, 0.2f, 0.2f),
        float3(0.2f, 1.0f, 0.2f),
        float3(0.2f, 0.2f, 1.0f)
    };

    VSOut outVS;
    outVS.position = positions[vertexID % 3]; // each patch uses 3 control points
    outVS.color = colors[vertexID % 3];
    return outVS;
}

struct HSOut {
    float3 position : POSITION;
    float3 color    : COLOR0;
};

// Patch constant structure that provides tess factors
struct HSPatchConst {
    float edges[3]  : SV_TessFactor;        // outer tess factors (edges)
    float inside    : SV_InsideTessFactor;  // inner tess factor
};

// Hull shader: output control points for the patch
[domain("tri")]
[partitioning("integer")]           // choose partitioning mode: integer / fractional_odd / fractional_even
[outputtopology("triangle_cw")]    // output primitive topology
[outputcontrolpoints(3)]           // number of control points we output
[patchconstantfunc("PatchConstants")]
HSOut hullMain(InputPatch<VSOut, 3> patch, uint cpID : SV_OutputControlPointID)
{
    HSOut outHS;
    outHS.position = patch[cpID].position; // pass-through control point
    outHS.color    = patch[cpID].color;
    return outHS;
}

// Patch constant function: set tessellation levels here (can be dynamic)
HSPatchConst PatchConstants(InputPatch<VSOut, 3> patch)
{
    HSPatchConst pc;
    // Example: fairly high subdivision to make tessellation visible
    pc.edges[0] = 8.0f;
    pc.edges[1] = 8.0f;
    pc.edges[2] = 8.0f;
    pc.inside   = 8.0f;
    return pc;
}

struct DSOut {
    float4 position : SV_Position;
    float3 color    : COLOR0;
};

[domain("tri")]
DSOut domainMain(HSPatchConst patchConsts, const OutputPatch<HSOut, 3> patch, float3 bary : SV_DomainLocation)
{
    // bary = barycentric coordinates of the tessellated vertex
    // Interpolate control point positions / attributes using barycentric coords
    float3 pos = bary.x * patch[0].position + bary.y * patch[1].position + bary.z * patch[2].position;

    float3 col = bary.x * patch[0].color + bary.y * patch[1].color + bary.z * patch[2].color;

    DSOut outDS;
    outDS.position = float4(pos, 1.0f);
    outDS.color = col;
    return outDS;
}

float4 fragmentMain(DSOut input) : SV_Target
{
    return float4(input.color, 1.0f);
}
