#ifndef PYRO_COMMON__UNIFORM_BUFFER_VIEW_SLANG
#define PYRO_COMMON__UNIFORM_BUFFER_VIEW_SLANG

#define PYRO_BIND_UNIFORM_BUFFER(slot, type, name) [[vk::binding(slot, 1)]] cbuffer pyro_Uniform##name : register(b##slot) { type name; }

#endif 
