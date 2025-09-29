#pragma once
#include <PyroRHI/Shader/IShaderFeatureSet.hpp>
#include <ShockGraph/Resources.hpp>

#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <slang-com-ptr.h>
#include <EASTL/string_map.h>

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
         ~ShaderCompiler() ;

        TaskShader CompileShaderFromFile(const eastl::string& path, const ShaderCompilationInfo& info);
        TaskShader CompileShaderFromSource(const eastl::string& code, const ShaderCompilationInfo& info, const eastl::string& virtualSourcePath = {});

    private:
        void RegisterShader(TaskShader_* shader);
        void UnregisterShader(TaskShader_* shader);


     slang::IGlobalSession* mGlobalSession = nullptr;
        const IShaderFeatureSet* mFeatureSet;
        TaskResourceManager& mResourceManager;
    };

}