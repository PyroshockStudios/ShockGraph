cmake_minimum_required(VERSION 3.28)
project(ShockGraph)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

foreach(OUTPUTCONFIG Debug Release RelWithDebInfo MinSizeRel)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_BINARY_DIR}/lib")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_BINARY_DIR}/bin")
    set(CMAKE_EXECUTABLE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_BINARY_DIR}/bin")
endforeach()

if (MSVC)
add_compile_options(
 /Zc:preprocessor   # libassert preprocessor issues
 /MP                # multithreading 
)

add_compile_options()
endif()

include(Config.cmake)

# ==== Project Config ====

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(vendor)

add_subdirectory(ShockGraph)

if(SHOCKGRAPH_BUILD_VISUAL_TESTS)
add_subdirectory(VisualTests)
endif()


set(COMPILE_COMMANDS_FILE "${CMAKE_BINARY_DIR}/compile_commands.json")
set(DESTINATION_FILE "${CMAKE_SOURCE_DIR}/compile_commands.json")

if(EXISTS "${CMAKE_BINARY_DIR}/compile_commands.json")
  # Add a custom command to copy compile_commands.json after the build
  add_custom_command(OUTPUT "${CMAKE_SOURCE_DIR}/compile_commands.json"
    COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_BINARY_DIR}/compile_commands.json"
            "${CMAKE_SOURCE_DIR}"
    DEPENDS "${CMAKE_BINARY_DIR}/compile_commands.json"
    COMMENT "Copying compile_commands.json to source directory"
  )

  # Make sure the custom command runs as part of the build. This attaches the
  # command to the ALL target, meaning it runs every time you build the project.
  add_custom_target(CopyCompileCommands ALL
    DEPENDS "${CMAKE_SOURCE_DIR}/compile_commands.json"
  )
else()
  message(STATUS "compile_commands.json not found in build directory.  Skipping copy.")
endif()
