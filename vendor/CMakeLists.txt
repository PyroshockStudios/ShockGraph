# common is included in the following two
include(FetchContent)
FetchContent_Declare(
    PyroCommon
    GIT_REPOSITORY https://github.com/PyroshockStudios/PyroCommon.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)
if (SHOCKGRAPH_USE_PYRO_PLATFORM)
    if(NOT TARGET PyroPlatform)
        # find the package locally first
        find_package(PyroPlatform CONFIG QUIET)
        # If not found, fetch it directly from repository
        if(NOT PyroPlatform_FOUND)
            message(STATUS "PyroPlatform not found locally. Fetching from GitHub...")
            FetchContent_Declare(
                PyroPlatform
                GIT_REPOSITORY https://github.com/PyroshockStudios/PyroPlatform
                GIT_TAG        master
            )
            FetchContent_MakeAvailable(PyroPlatform)
        endif()
    endif()
endif()

# RHI

if(NOT TARGET PyroRHI)
    # find the package locally first
    find_package(PyroRHI CONFIG QUIET)
    # If not found, fetch it directly from repository
    if(NOT PyroRHI_FOUND)
        message(STATUS "PyroPlatform not found locally. Fetching from GitHub...")
        FetchContent_Declare(
            PyroRHI
            GIT_REPOSITORY https://github.com/PyroshockStudios/PyroRHI
            GIT_TAG        master
        )
        FetchContent_MakeAvailable(PyroRHI)
    endif()
endif()
