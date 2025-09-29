#ifndef PYRO_COMMON__SPECIALIZATION_CONSTANT_SLANG
#define PYRO_COMMON__SPECIALIZATION_CONSTANT_SLANG

#ifdef PYRO_SHADER_FLAG_ENABLE_SPECIALIZATION_CONSTANTS
#define PYRO_SPECIALIZATION(slot, type, name, defaultValue) [[vk::constant_id(slot)]] const type name = (type)(defaultValue);
#else

#ifdef pyro_internal_shader_stage_cs
#define pyro_internal_spec_const_Register_For_Stage b8
#elif defined(pyro_internal_shader_stage_vs)
#define pyro_internal_spec_const_Register_For_Stage b8
#elif defined(pyro_internal_shader_stage_hs)
#define pyro_internal_spec_const_Register_For_Stage b9
#elif defined(pyro_internal_shader_stage_ds)
#define pyro_internal_spec_const_Register_For_Stage b10
#elif defined(pyro_internal_shader_stage_gs)
#define pyro_internal_spec_const_Register_For_Stage b11
#elif defined(pyro_internal_shader_stage_fs)
#define pyro_internal_spec_const_Register_For_Stage b12
#endif

cbuffer pyro_internal__PyroSpecConst : register(pyro_internal_spec_const_Register_For_Stage, space0) 
{
    uint4 pyro_internal__Specialization[16]; 
}

#define PYRO_SPECIALIZATION(slot, type, name, defaultValue) \
	static const type name = pyro_internal__Specialization[slot].y == 1 ? reinterpret<type>(pyro_internal__Specialization[slot].x) : (type)(defaultValue)

#endif

#endif 
