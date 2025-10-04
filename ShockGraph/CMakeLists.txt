set(SH_SRC ${CMAKE_CURRENT_SOURCE_DIR})
file(GLOB_RECURSE ENDF6_SRC
      "${SH_SRC}/*.hpp"
      "${SH_SRC}/*.cpp")


if (SHOCKGRAPH_SHARED_LIBRARY)
add_library(ShockGraph SHARED ${ENDF6_SRC})
target_compile_definitions(ShockGraph PUBLIC SHOCKGRAPH_SHARED_LIBRARY=1)
else()
add_library(ShockGraph STATIC ${ENDF6_SRC})
endif()
if (SHOCKGRAPH_USE_PYRO_PLATFORM)
   target_compile_definitions(ShockGraph PUBLIC SHOCKGRAPH_USE_PYRO_PLATFORM=1)
endif()
add_library(ShockGraph::ShockGraph ALIAS ShockGraph)

foreach(_source IN ITEMS ${ENDF6_SRC})
    get_filename_component(_source_path "${_source}" PATH)
    string(REPLACE "${SH_SRC}" "" _group_path "${_source_path}")
    string(REPLACE "/" "\\" _group_path "${_group_path}")
    source_group("${_group_path}" FILES "${_source}")
endforeach()

target_include_directories(ShockGraph
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
        $<INSTALL_INTERFACE:include>
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DESTINATION include/ShockGraph
    FILES_MATCHING PATTERN "*.hpp"
)

target_compile_features(ShockGraph PRIVATE cxx_std_23)

target_link_libraries(ShockGraph PUBLIC PyroRHI::PyroRHI)

if(SHOCKGRAPH_USE_PYRO_PLATFORM)
target_link_libraries(ShockGraph PUBLIC PyroPlatform::PyroPlatform)
endif()

# windows cope
set_target_properties(ShockGraph PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS YES)
set_target_properties(ShockGraph PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
