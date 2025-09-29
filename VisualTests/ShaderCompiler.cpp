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

#include "ShaderCompiler.hpp"
#include "Core.hpp"

#include <slang-com-helper.h>

#include <EASTL/hash_set.h>
#include <PyroCommon/Logger.hpp>
#include <PyroCommon/Serialization/BinarySerializer.hpp>
#include <PyroCommon/Stream/IStreamReader.hpp>
#include <PyroCommon/Stream/IStreamWriter.hpp>
#include <PyroRHI/Shader/IShaderFeatureSet.hpp>

#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>

#include <PyroCommon/Stream/FileStream.hpp>
#include <libassert/assert.hpp>

namespace VisualTests {
    using namespace Slang;

    ShaderCompiler::ShaderCompiler(TaskResourceManager& resourceManager, const IShaderFeatureSet* featureSet)
        : mResourceManager(resourceManager), mFeatureSet(featureSet) {
        SlangGlobalSessionDesc desc{};
        desc.structureSize = sizeof(desc);
        desc.apiVersion = SLANG_API_VERSION;
        desc.minLanguageVersion = SLANG_LANGUAGE_VERSION_2025;
        desc.enableGLSL = featureSet->Features().bGLSL;

        SlangResult result = slang::createGlobalSession(&desc, &mGlobalSession);
        ASSERT(result == 0);
    }

    ShaderCompiler::~ShaderCompiler() {
        mGlobalSession->release();
        mGlobalSession = nullptr;
    }

    TaskShader ShaderCompiler::CompileShaderFromFile(const eastl::string& path, const ShaderCompilationInfo& info) {
        eastl::string code{};
        std::filesystem::path abspath = std::filesystem::absolute(path.c_str());
        std::ifstream reader{ abspath, std::ios::in };
        std::stringstream stream{};
        stream << reader.rdbuf();
        code = stream.str().c_str();
        if (code.empty()) {
            Logger::Error(gShaderSink, "Failed to load code from " + eastl::string(abspath.string().c_str()));
            return {};
        }
        return CompileShaderFromSource(code, info, path);
    }

    TaskShader ShaderCompiler::CompileShaderFromSource(const eastl::string& code_, const ShaderCompilationInfo& info, const eastl::string& virtualSourcePath) {
        if (code_.empty()) {
            Logger::Error(gShaderSink, "Slang received empty code! Returning...");
            return {};
        }
        eastl::string code = code_;

        eastl::hash_set<eastl::string> pyroDrawIndexAliases = {};
        // eastl::hash_set<eastl::string> pyroFirstVertexAliases = {};
        // eastl::hash_set<eastl::string> pyroFirstInstanceAliases = {};
        if (!mFeatureSet->Features().bDrawParameters) {
            static auto IsInsideComment = [](const std::string& code, size_t pos) -> bool {
                // Check for line comments
                size_t lineStart = code.rfind('\n', pos);
                if (lineStart == std::string::npos)
                    lineStart = 0;
                if (code.find("//", lineStart) < pos)
                    return true;

                // Check for block comments
                size_t blockStart = code.rfind("/*", pos);
                size_t blockEnd = code.rfind("*/", pos);
                if (blockStart != std::string::npos && (blockEnd == std::string::npos || blockEnd < blockStart))
                    return true;

                return false;
            };
            std::string stdcode = std::string(code.c_str(), code.length());

            static auto MatchAndReplace = [](std::string find, std::string& outcode, eastl::hash_set<eastl::string>& outAliases) -> void {
                // std::regex paramPattern(R"((?:,\s*|\s*)([A-Za-z_]\w*)\s+([A-Za-z_]\w*)\s*:\s*SV_DrawIndex\s*(?:,)?))");
                std::regex paramPattern("(?:,\\s*|\\s*)([A-Za-z_]\\w*)\\s+([A-Za-z_]\\w*)\\s*:\\s*" + find + "\\s*(?:,)?");

                std::smatch match;
                while (std::regex_search(outcode, match, paramPattern)) {
                    size_t pos = match.position(0);
                    std::string matchStr = match[0];

                    // Skip if inside comment
                    if (IsInsideComment(outcode, pos)) {
                        // Move past this match without replacing
                        outcode = outcode.substr(0, pos + 1) + outcode.substr(pos + 1);
                        continue;
                    }

                    std::string varName = match[2];
                    // Logger::Info("Removed variable: " + eastl::string(varName.c_str()));
                    outAliases.emplace(varName.c_str());

                    // Preserve newlines
                    std::string newlines;
                    for (char c : matchStr) {
                        if (c == '\n')
                            newlines.push_back('\n');
                    }

                    bool hasLeadingComma = !matchStr.empty() && matchStr[0] == ',';
                    bool hasTrailingComma = !matchStr.empty() && matchStr.back() == ',';

                    std::string replacement = newlines;
                    if (hasLeadingComma && hasTrailingComma) {
                        replacement = "," + newlines; // Keep one comma in the middle
                    }

                    outcode.replace(pos, match.length(0), replacement);
                }
            };

            // Find SV_DrawIndex and replace it with a reference to the constant buffer if unsupported
            MatchAndReplace("SV_DrawIndex", stdcode, pyroDrawIndexAliases);
            // MatchAndReplace("SV_StartVertexLocation", stdcode, pyroFirstVertexAliases);
            // MatchAndReplace("SV_StartInstanceLocation", stdcode, pyroFirstInstanceAliases);
            code = eastl::string(stdcode.c_str(), stdcode.length());
            //  Logger::Info(code);
        }

        eastl::vector<slang::PreprocessorMacroDesc> macros = {};
        for (const auto& macro : mFeatureSet->GlobalPreprocessorDefines()) {
            macros.emplace_back(macro.first, macro.second);
        }
        for (const auto& drawIndexAlias : pyroDrawIndexAliases) {
            macros.emplace_back(drawIndexAlias.c_str(), "pyro_internal__DrawIndex");
        }
        // for (const auto& drawIndexAlias : pyroFirstVertexAliases) {
        //     macros.emplace_back(drawIndexAlias.c_str(), "pyro_internal__FirstVertex");
        // }
        // for (const auto& drawIndexAlias : pyroFirstInstanceAliases) {
        //     macros.emplace_back(drawIndexAlias.c_str(), "pyro_internal__FirstInstance");
        // }


        eastl::vector<std::string> includes = {};
        eastl::vector<const char*> includesPtr = {};
        std::filesystem::path absoluteIncludeDir = std::filesystem::absolute("resources/Shaders/Include");
        includes.push_back(absoluteIncludeDir.string());
        includesPtr.push_back(includes.back().c_str());

        const char* profileName = mFeatureSet->GetProfileName(info.stage);
        slang::TargetDesc targetDesc = {};
        targetDesc.format = (SlangCompileTarget)mFeatureSet->GetTarget();
        targetDesc.profile = mGlobalSession->findProfile(profileName);
        targetDesc.forceGLSLScalarBufferLayout = mFeatureSet->Features().bScalarLayout;

        slang::SessionDesc sessionDesc = {};
        sessionDesc.targets = &targetDesc;
        sessionDesc.targetCount = 1;
        sessionDesc.searchPaths = includesPtr.data();
        sessionDesc.searchPathCount = static_cast<SlangInt>(includesPtr.size());
        // sessionDesc.fileSystem = gSlangFileSystem.get();
        sessionDesc.preprocessorMacros = macros.data();
        sessionDesc.preprocessorMacroCount = static_cast<SlangInt>(macros.size());
        sessionDesc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;
        using namespace slang;

        slang::ISession* session = nullptr;

        SlangResult result = {};
        result = mGlobalSession->createSession(sessionDesc, &session);
        ASSERT(result == 0);

        Slang::ComPtr<SlangCompileRequest> slangRequest = nullptr;
        result = session->createCompileRequest(slangRequest.writeRef());
        ASSERT(result == 0);

        if (slangRequest == nullptr) {
            Logger::Error(gShaderSink, "Slang failed to create a valid compiler request!");
            return {};
        }

        eastl::vector<const char*> args = {
            // https://github.com/shader-slang/slang/issues/3532
            // Disables warning for aliasing bindings.
            // clang-format off
                "-warnings-disable", "39001",
                "-O0",
                "-g2",
            // clang-format on
        };
        result = slangRequest->processCommandLineArguments(args.data(), args.size());
        ASSERT(result == 0);

        for (const auto& [key, value] : info.defines) {
            slangRequest->addPreprocessorDefine(key.c_str(), value.c_str());
        }
        switch (info.stage) {
        case ShaderStage::Vertex:
            slangRequest->addPreprocessorDefine("pyro_internal_shader_stage_vs", "1");
            break;
        case ShaderStage::Hull:
            slangRequest->addPreprocessorDefine("pyro_internal_shader_stage_hs", "1");
            break;
        case ShaderStage::Domain:
            slangRequest->addPreprocessorDefine("pyro_internal_shader_stage_ds", "1");
            break;
        case ShaderStage::Geometry:
            slangRequest->addPreprocessorDefine("pyro_internal_shader_stage_gs", "1");
            break;
        case ShaderStage::Fragment:
            slangRequest->addPreprocessorDefine("pyro_internal_shader_stage_fs", "1");
            break;
        case ShaderStage::Compute:
            slangRequest->addPreprocessorDefine("pyro_internal_shader_stage_cs", "1");
            break;
        default:
            ASSERT(false, "TODO");
            break;
        }
        if (mFeatureSet->Features().bGLSL) {
            slangRequest->addPreprocessorDefine("pyro_internal_enabled_glsl", "1");
        }

        for (const auto& file : std::filesystem::recursive_directory_iterator(absoluteIncludeDir)) {
            if (!file.is_regular_file())
                continue;
            int virtualFileIndex = slangRequest->addTranslationUnit(SLANG_SOURCE_LANGUAGE_SLANG, file.path().string().c_str());
            std::ifstream reader{ file.path(), std::ios::in };
            std::stringstream stream{};
            stream << reader.rdbuf();

            std::string relativePathInInclude = std::filesystem::relative(file.path(), absoluteIncludeDir).string();
            if (stream.eof()) {
                Logger::Warn(gShaderSink, "Failed to load code from '" + eastl::string(relativePathInInclude.c_str()) + "'. Ignoring file...");
            }

            slangRequest->addTranslationUnitSourceString(virtualFileIndex, relativePathInInclude.c_str(), stream.str().c_str());
        }

        int translationUnitIndex = slangRequest->addTranslationUnit(SLANG_SOURCE_LANGUAGE_SLANG, info.name.c_str());
        slangRequest->addTranslationUnitSourceString(translationUnitIndex, virtualSourcePath.empty() ? "PyroShader" : virtualSourcePath.c_str(), code.c_str());
        slangRequest->addEntryPoint(translationUnitIndex, info.entryPoint.c_str(), static_cast<SlangStage>(info.stage));
        result = slangRequest->compile();
        const char* diagnostics = slangRequest->getDiagnosticOutput();
        if (diagnostics && strlen(diagnostics) > 0) {
            if (SLANG_FAILED(result)) {
                Logger::Error(gShaderSink, "Slang failed to compile a shader! Diagnostics: " + eastl::string(diagnostics));
            } else {
                Logger::Warn(gShaderSink, "Slang compiled shader successfully, but generated diagnostics: " + eastl::string(diagnostics));
            }
        }
        ASSERT(result == 0);
        if (SLANG_FAILED(result)) {
            return {};
        }

        Slang::ComPtr<slang::IModule> shaderModule = {};
        slangRequest->getModule(translationUnitIndex, shaderModule.writeRef());

        Slang::ComPtr<slang::IEntryPoint> entryp;
        result = shaderModule->getDefinedEntryPoint(0, entryp.writeRef());
        ASSERT(result == 0);
        Slang::ComPtr<slang::IBlob> bytecode;
        result = slangRequest->getEntryPointCodeBlob(0, 0, bytecode.writeRef());

        ShaderProgram program{};
        program.bytecode.resize(bytecode->getBufferSize());
        memcpy(program.bytecode.data(), bytecode->getBufferPointer(), bytecode->getBufferSize());

        TaskShader shader = TaskShader::Create(eastl::move(program), [](void* this_, TaskShader_* shader) { reinterpret_cast<ShaderCompiler*>(this_)->UnregisterShader(shader); }, this);
        RegisterShader(shader.Get());

        session->release();
        return shader;
    }
    void ShaderCompiler::RegisterShader(TaskShader_* shader) {
        // TODO
    }
    void ShaderCompiler::UnregisterShader(TaskShader_* shader) {
        // TODO
    }
} // namespace VisualTests
