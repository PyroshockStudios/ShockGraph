#ifndef PYRO_COMMON__PUSH_CONSTANT_SLANG
#define PYRO_COMMON__PUSH_CONSTANT_SLANG

#define PYRO_PUSH_CONSTANT(type, name) [[push_constant]] cbuffer pyro_PushConstant##name : register(b13, space0) { type name; }

#endif 
