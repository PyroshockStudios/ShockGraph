// MIT License
//
// Copyright (c) 2025 Pyroshock Studios
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once
#include <PyroRHI/Shader/IShaderFeatureSet.hpp>
#include <ShockGraph/Resources.hpp>

#include <EASTL/string.h>
#include <EASTL/string_map.h>
#include <EASTL/vector.h>
#include <slang-com-ptr.h>

using namespace PyroshockStudios;
using namespace PyroshockStudios::Types;
using namespace PyroshockStudios::RHI;
using namespace PyroshockStudios::Renderer;
namespace VisualTests {
    struct ShaderCompilationInfo {
        ShaderStage stage = {};
        eastl::string entryPoint = "main";
        eastl::vector<eastl::pair<eastl::string /*macro*/, eastl::string /*value*/>> defines = {};
        eastl::string name = "PyroShader";

        inline bool operator==(const ShaderCompilationInfo& o) const noexcept {
            return stage == o.stage && entryPoint == o.entryPoint && defines == o.defines && name == o.name;
        }
        inline bool operator!=(const ShaderCompilationInfo& o) const noexcept {
            return !(*this == o);
        }
    };

    class ShaderCompiler : DeleteCopy, DeleteMove {
    public:
        ShaderCompiler(TaskResourceManager& resourceManager, const IShaderFeatureSet* featureSet);
        ~ShaderCompiler();

        TaskShader CompileShaderFromFile(const eastl::string& path, const ShaderCompilationInfo& info);
        TaskShader CompileShaderFromSource(const eastl::string& code, const ShaderCompilationInfo& info, const eastl::string& virtualSourcePath = {});

    private:
        void RegisterShader(TaskShader_* shader);
        void UnregisterShader(TaskShader_* shader);


        slang::IGlobalSession* mGlobalSession = nullptr;
        const IShaderFeatureSet* mFeatureSet;
        TaskResourceManager& mResourceManager;
    };

} // namespace VisualTests