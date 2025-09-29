#ifndef PYRO_COMMON__UNORDERED_ACCESS_VIEW_SLANG
#define PYRO_COMMON__UNORDERED_ACCESS_VIEW_SLANG

#define PYRO_BIND_UNORDERED_ACCESS_STRUCTURED_BUFFER(slot, type, name)      [[vk::binding(slot + 8, 1)]] RWStructuredBuffer<type> name : register(u##slot, space1)
#define PYRO_BIND_UNORDERED_ACCESS_BYTE_ADDRESS_BUFFER(slot, name)    		[[vk::binding(slot + 8, 1)]] RWByteAddressBuffer name : register(u##slot, space1)
#define PYRO_BIND_UNORDERED_ACCESS_TEXTURE_1D(slot, type, name)             [[vk::binding(slot + 20, 1)]] RWTexture1D<type> name : register(u##slot, space1)
#define PYRO_BIND_UNORDERED_ACCESS_TEXTURE_2D(slot, type, name)             [[vk::binding(slot + 20, 1)]] RWTexture2D<type> name : register(u##slot, space1)
#define PYRO_BIND_UNORDERED_ACCESS_TEXTURE_3D(slot, type, name)             [[vk::binding(slot + 20, 1)]] RWTexture3D<type> name : register(u##slot, space1)
#define PYRO_BIND_UNORDERED_ACCESS_TEXTURE_1D_ARRAY(slot, type, name)       [[vk::binding(slot + 20, 1)]] RWTexture1DArray<type> name : register(u##slot, space1)
#define PYRO_BIND_UNORDERED_ACCESS_TEXTURE_2D_ARRAY(slot, type, name)       [[vk::binding(slot + 20, 1)]] RWTexture2DArray<type> name : register(u##slot, space1)

#endif 
