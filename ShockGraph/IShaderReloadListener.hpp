#pragma once

#include <PyroCommon/GUID.hpp>
#include <PyroRHI/Shader/ShaderProgram.hpp>

#include <EASTL/functional.h>
#include <EASTL/string.h>

namespace PyroshockStudios {
    inline namespace Renderer {
        typedef struct TaskShader_* TaskShaderHandle;
        struct IShaderReloadListener {
            IShaderReloadListener() = default;
            virtual ~IShaderReloadListener() = default;

            virtual void OnShaderChange(TaskShaderHandle shader, ShaderProgram&& newProgram) = 0;
        };
    }
}