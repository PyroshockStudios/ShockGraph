#pragma once
#include <PyroCommon/Platform.hpp>

#ifdef _MSC_VER
#ifdef SHOCKGRAPH_SHARED_LIBRARY
#ifdef ShockGraph_EXPORTS
#define SHOCKGRAPH_API __declspec(dllexport)
#else
#define SHOCKGRAPH_API __declspec(dllimport)
#endif
#else
#define SHOCKGRAPH_API
#endif
#else
#define SHOCKGRAPH_API
#endif