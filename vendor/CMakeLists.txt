# Common

if(NOT TARGET PyroCommon)
    find_package(PyroCommon CONFIG QUIET)
    if(NOT PyroCommon_FOUND)
        set(PYRO_COMMON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/PyroCommon" CACHE PATH "Path to PyroCommon")
        if(NOT EXISTS "${PYRO_COMMON_DIR}/CMakeLists.txt")
            message(FATAL_ERROR "PyroCommon not found at ${PYRO_COMMON_DIR}")
        endif()
        set(PYRO_COMMON_BINARY_DIR "${CMAKE_BINARY_DIR}/PyroCommon")
        add_subdirectory(${PYRO_COMMON_DIR} ${PYRO_COMMON_BINARY_DIR})
    endif()
endif()

# Platform


if(NOT TARGET PyroPlatform)
    find_package(PyroPlatform CONFIG QUIET)
    if(NOT PyroPlatform_FOUND)
        set(PYRO_PLATFORM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/PyroPlatform" CACHE PATH "Path to PyroPlatform")
        if(NOT EXISTS "${PYRO_PLATFORM_DIR}/CMakeLists.txt")
            message(FATAL_ERROR "PyroPlatform not found at ${PYRO_PLATFORM_DIR}")
        endif()
        set(PYRO_PLATFORM_BINARY_DIR "${CMAKE_BINARY_DIR}/PyroPlatform")
        add_subdirectory(${PYRO_PLATFORM_DIR} ${PYRO_PLATFORM_BINARY_DIR})
    endif()
endif()

# RHI

if(NOT TARGET PyroRHI)
    find_package(PyroRHI CONFIG QUIET)
    if(NOT PyroRHI_FOUND)
        set(PYRO_RHI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/PyroRHI" CACHE PATH "Path to PyroRHI")
        if(NOT EXISTS "${PYRO_RHI_DIR}/CMakeLists.txt")
            message(FATAL_ERROR "PyroRHI not found at ${PYRO_RHI_DIR}")
        endif()
        set(PYRO_RHI_BINARY_DIR "${CMAKE_BINARY_DIR}/PyroRHI")
        add_subdirectory(${PYRO_RHI_DIR} ${PYRO_RHI_BINARY_DIR})
    endif()
endif()