set(SH_SRC ${CMAKE_SOURCE_DIR}/VisualTests)
file(GLOB_RECURSE ENDF6_SRC
      "${SH_SRC}/*.hpp"
      "${SH_SRC}/*.cpp")

add_executable(SGVisualTests ${ENDF6_SRC})

if (MSVC)
set_target_properties(SGVisualTests PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endif()

foreach(_source IN ITEMS ${ENDF6_SRC})
    get_filename_component(_source_path "${_source}" PATH)
    string(REPLACE "${SH_SRC}" "" _group_path "${_source_path}")
    string(REPLACE "/" "\\" _group_path "${_group_path}")
    source_group("${_group_path}" FILES "${_source}")
endforeach()

include(FetchContent)
    set(SLANG_VERSION "2025.17") 
    set(SLANG_ROOT "${CMAKE_BINARY_DIR}/_deps/slang")
    set(SLANG_URL_BASE "https://github.com/shader-slang/slang/releases/download/v${SLANG_VERSION}")

    if(WIN32)
        set(SLANG_ARCHIVE "slang-${SLANG_VERSION}-windows-x86_64.zip")
    elseif(UNIX AND NOT APPLE)
        set(SLANG_ARCHIVE "slang-${SLANG_VERSION}-linux-x86_64.zip")
    elseif(APPLE)
        # Detect architecture: x86_64 or arm64
        execute_process(
            COMMAND uname -m
            OUTPUT_VARIABLE ARCHITECTURE
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if(ARCHITECTURE STREQUAL "x86_64")
            set(SLANG_ARCHIVE "slang-${SLANG_VERSION}-macos-x86_64.zip")
        elseif(ARCHITECTURE STREQUAL "arm64")
            set(SLANG_ARCHIVE "slang-${SLANG_VERSION}-macos-aarch64.zip")
        else()
            message(FATAL_ERROR "Unsupported macOS architecture: ${ARCHITECTURE}")
        endif()
    else()
        message(FATAL_ERROR "Unsupported platform")
    endif()

    set(SLANG_URL "${SLANG_URL_BASE}/${SLANG_ARCHIVE}")
    set(SLANG_DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/_downloads")
    set(SLANG_EXTRACT_DIR "${SLANG_ROOT}")

    if(NOT EXISTS "${SLANG_EXTRACT_DIR}/include/slang.h")
        message(STATUS "Downloading Slang SDK from ${SLANG_URL}...")
        file(DOWNLOAD "${SLANG_URL}" "${SLANG_DOWNLOAD_DIR}/${SLANG_ARCHIVE}" SHOW_PROGRESS)
        message(STATUS "Extracting Slang SDK...")
        file(MAKE_DIRECTORY "${SLANG_EXTRACT_DIR}")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xzf "${SLANG_DOWNLOAD_DIR}/${SLANG_ARCHIVE}"
            WORKING_DIRECTORY "${SLANG_EXTRACT_DIR}"
            RESULT_VARIABLE _extract_result
        )
        if(NOT _extract_result EQUAL 0)
            message(FATAL_ERROR "Failed to extract Slang SDK archive")
        endif()
    endif()

    target_include_directories(SGVisualTests PRIVATE "${SLANG_EXTRACT_DIR}/include")
    target_link_directories(SGVisualTests PRIVATE "${SLANG_EXTRACT_DIR}/lib")

    set(SLANG_LIBS slang )
    if(UNIX AND NOT APPLE)
        list(APPEND SLANG_LIBS pthread dl)
    endif()

    target_link_libraries(SGVisualTests PUBLIC ${SLANG_LIBS})

# get slang
file(GLOB SLANG_COPYLIBS
	"${CMAKE_BINARY_DIR}/_deps/slang/bin/*.dll"
	"${CMAKE_BINARY_DIR}/_deps/slang/bin/*.exe"
	"${CMAKE_BINARY_DIR}/_deps/slang/bin/*.slang"
)
message(STATUS "SLANG_COPYLIBS = ${SLANG_COPYLIBS}")
  # copy slang binaries
foreach(slang_lib ${SLANG_COPYLIBS})
	add_custom_command(TARGET SGVisualTests POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		${slang_lib} $<TARGET_FILE_DIR:SGVisualTests>
)
endforeach()

target_link_libraries(SGVisualTests PRIVATE ShockGraph::ShockGraph)

target_compile_features(SGVisualTests PRIVATE cxx_std_23)
set_target_properties(SGVisualTests PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
