#ifndef PYRO_COMMON__INTRINSICS_SLANG
#define PYRO_COMMON__INTRINSICS_SLANG

#ifdef pyro_internal_enabled_glsl
	import glsl;
	#version 460;
#endif

#ifdef pyro_internal_shader_stage_vs
	#ifdef PYRO_SHADER_FLAG_ENABLE_GL_DRAW_ID
		//#define pyro_internal__DrawIndex gl_DrawID
	#else
		[[push_constant]] cbuffer pyro_internal__cbuffer_DrawIndex : register(b0, space2) { uint pyro_internal__DrawIndex; };
	#endif
#else
	#define pyro_internal__DrawIndex 0
#endif


#endif