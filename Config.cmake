# ==== Test config ====
option(SHOCKGRAPH_BUILD_RENDERGRAPH_TESTS "Build rendergraph tests" OFF) 
option(SHOCKGRAPH_BUILD_VISUAL_TESTS "Build visual tests (requires PyroPlatform)" OFF) 
option(SHOCKGRAPH_SHARED_LIBRARY "Build ShockGraph as shared library" OFF) 
option(SHOCKGRAPH_USE_PYRO_PLATFORM "Use PyroPlaform for the SwapChain abstraction" ON) 

if (SHOCKGRAPH_BUILD_VISUAL_TESTS)
    set(SHOCKGRAPH_USE_PYRO_PLATFORM ON CACHE BOOL "Use PyroPlaform for the SwapChain abstraction")
endif()