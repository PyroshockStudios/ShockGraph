#ifndef COMMON__DESCRIPTOR_INDEXING_SLANG
#define COMMON__DESCRIPTOR_INDEXING_SLANG

typedef uint32_t PyroDescriptor;

#define pyro_internal_access(index, resource) resource[index]
#define PYRO_ACCESS(index, resource) pyro_internal_access(index, resource)
// Byte address buffers
[[vk::binding(0)]] ByteAddressBuffer        gByteBuffer[]        : register(t0, space1);

// Textures (SRVs)
[[vk::binding(1)]] Texture1D<float4>        gTexture1D[]         : register(t0, space1);
[[vk::binding(1)]] Texture2D<float4>        gTexture2D[]         : register(t0, space1);
[[vk::binding(1)]] TextureCube<float4>      gTextureCube[]       : register(t0, space1);
[[vk::binding(1)]] Texture3D<float4>        gTexture3D[]         : register(t0, space1);
[[vk::binding(1)]] Texture1DArray<float4>   gTexture1DArray[]    : register(t0, space1);
[[vk::binding(1)]] Texture2DArray<float4>   gTexture2DArray[]    : register(t0, space1);
[[vk::binding(1)]] TextureCubeArray<float4> gTextureCubeArray[]  : register(t0, space1);
[[vk::binding(1)]] Texture2DMS<float4>      gTexture2DMS[]       : register(t0, space1);
[[vk::binding(1)]] Texture2DMSArray<float4> gTexture2DMSArray[]  : register(t0, space1);

// Samplers
[[vk::binding(2)]] SamplerState             gSampler[]           : register(s0, space1);
[[vk::binding(2)]] SamplerComparisonState   gSamplerComparison[] : register(s0, space1);


#endif 
