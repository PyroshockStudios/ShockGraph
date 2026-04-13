#ifndef COMMON__DESCRIPTOR_INDEXING_SLANG
#define COMMON__DESCRIPTOR_INDEXING_SLANG

typedef uint32_t PyroDescriptor;

#define pyro_internal_access(index, resource) resource[index]
#define PYRO_ACCESS(index, resource) pyro_internal_access(index, resource)
// Byte address buffers
[[vk::binding(0)]] ByteAddressBuffer        gByteBuffer[]        : register(t0, space1);

// Textures (SRVs)
[[vk::binding(1)]] Texture1D<float4>        gTexture1D[]         : register(t0, space1);
[[vk::binding(1)]] Texture2D<float4>        gTexture2D[]         : register(t0, space2);
[[vk::binding(1)]] TextureCube<float4>      gTextureCube[]       : register(t0, space3);
[[vk::binding(1)]] Texture3D<float4>        gTexture3D[]         : register(t0, space4);
[[vk::binding(1)]] Texture1DArray<float4>   gTexture1DArray[]    : register(t0, space5);
[[vk::binding(1)]] Texture2DArray<float4>   gTexture2DArray[]    : register(t0, space6);
[[vk::binding(1)]] TextureCubeArray<float4> gTextureCubeArray[]  : register(t0, space7);
[[vk::binding(1)]] Texture2DMS<float4>      gTexture2DMS[]       : register(t0, space8);
[[vk::binding(1)]] Texture2DMSArray<float4> gTexture2DMSArray[]  : register(t0, space9);

// Unsigned Textures (SRVs)
[[vk::binding(1)]] Texture1D<uint4>        gTexture1Du[]         : register(t0, space1);
[[vk::binding(1)]] Texture2D<uint4>        gTexture2Du[]         : register(t0, space2);
[[vk::binding(1)]] TextureCube<uint4>      gTextureCubeu[]       : register(t0, space3);
[[vk::binding(1)]] Texture3D<uint4>        gTexture3Du[]         : register(t0, space4);
[[vk::binding(1)]] Texture1DArray<uint4>   gTexture1DArrayu[]    : register(t0, space5);
[[vk::binding(1)]] Texture2DArray<uint4>   gTexture2DArrayu[]    : register(t0, space6);
[[vk::binding(1)]] TextureCubeArray<uint4> gTextureCubeArrayu[]  : register(t0, space7);
[[vk::binding(1)]] Texture2DMS<uint4>      gTexture2DMSu[]       : register(t0, space8);
[[vk::binding(1)]] Texture2DMSArray<uint4> gTexture2DMSArrayu[]  : register(t0, space9);

// Signed Textures (SRVs)
[[vk::binding(1)]] Texture1D<int4>        gTexture1Di[]         : register(t0, space1);
[[vk::binding(1)]] Texture2D<int4>        gTexture2Di[]         : register(t0, space2);
[[vk::binding(1)]] TextureCube<int4>      gTextureCubei[]       : register(t0, space3);
[[vk::binding(1)]] Texture3D<int4>        gTexture3Di[]         : register(t0, space4);
[[vk::binding(1)]] Texture1DArray<int4>   gTexture1DArrayi[]    : register(t0, space5);
[[vk::binding(1)]] Texture2DArray<int4>   gTexture2DArrayi[]    : register(t0, space6);
[[vk::binding(1)]] TextureCubeArray<int4> gTextureCubeArrayi[]  : register(t0, space7);
[[vk::binding(1)]] Texture2DMS<int4>      gTexture2DMSi[]       : register(t0, space8);
[[vk::binding(1)]] Texture2DMSArray<int4> gTexture2DMSArrayi[]  : register(t0, space9);

// Samplers
[[vk::binding(2)]] SamplerState             gSampler[]           : register(s0, space30);
[[vk::binding(2)]] SamplerComparisonState   gSamplerComparison[] : register(s0, space31);

// Acceleration Structures
[[vk::binding(3)]] RaytracingAccelerationStructure 	gTlas[] 		 		: register(t0, space1);

#endif 
