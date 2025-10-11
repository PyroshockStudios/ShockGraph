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

#include "TesselationShader.hpp"
#include <PyroRHI/Api/IDevice.hpp>
#include <PyroRHI/Context.hpp>

namespace VisualTests {
    void TesselationShader::CreateResources(const CreateResourceInfo& info) {
        image = info.resourceManager.CreatePersistentImage({
            .format = Format::RGBA8Unorm,
            .size = { info.displayInfo.width, info.displayInfo.height },
            .usage = ImageUsageFlagBits::RENDER_TARGET | ImageUsageFlagBits::TRANSFER_SRC | ImageUsageFlagBits::BLIT_SRC,
            .name = "Tesselation Shader Render Image",
        });
        target = info.resourceManager.CreateColorTarget({
            .image = image,
            .name = "Tesselation Shader RT",
        });
        vsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/TesselationShader.slang",
            { .stage = ShaderStage::Vertex, .entryPoint = "vertexMain", .name = "Tesselation Shader Vsh" });
        if (info.resourceManager.GetInternalContext()->Properties().bTesselationShader) {
            hsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/TesselationShader.slang",
                { .stage = ShaderStage::Hull, .entryPoint = "hullMain", .name = "Tesselation Shader Hsh" });
            dsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/TesselationShader.slang",
                { .stage = ShaderStage::Domain, .entryPoint = "domainMain", .name = "Tesselation Shader Dsh" });
        }
        fsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/TesselationShader.slang",
            { .stage = ShaderStage::Fragment, .entryPoint = "fragmentMain", .name = "Tesselation Shader Fsh" });
        pipeline = info.resourceManager.CreateRasterPipeline(
            {
                .colorTargetStates = { { .format = image->Info().format } },
                .tesselationState = { { .controlPoints = 3 } },
                .inputAssemblyState = { .primitiveTopology = info.resourceManager.GetInternalContext()->Properties().bTesselationShader ? PrimitiveTopology::PatchList : PrimitiveTopology::TriangleList },
                .rasterizerState = { .polygonMode = PolygonMode::Line },
                .name = "Raster Pipeline",
            },
            {
                .vertexShaderInfo = { TaskShaderInfo{ .program = vsh } },
                .hullShaderInfo = hsh ? eastl::optional{ TaskShaderInfo{ .program = hsh } } : eastl::nullopt,
                .domainShaderInfo = dsh ? eastl::optional{ TaskShaderInfo{ .program = dsh } } : eastl::nullopt,
                .fragmentShaderInfo = { TaskShaderInfo{ .program = fsh } },
            });
    }
    void TesselationShader::ReleaseResources(const ReleaseResourceInfo& info) {
        image = {};
        target = {};
        vsh = {};
        hsh = {};
        dsh = {};
        fsh = {};
        pipeline = {};
    }
    eastl::span<GenericTask*> TesselationShader::CreateTasks() {
        tasks = {
            new GraphicsCallbackTask(
                { .name = "Tesselation Shader", .color = LabelColor::GREEN },
                [this](GraphicsTask& task) {
                    task.BindColorTarget({
                        .target = target,
                        .clear = { { 0.0f, 0.0f, 0.0f, 1.0f } },
                    });
                },
                [this](TaskCommandList& commands) {
                    commands.SetRasterPipeline(pipeline);
                    commands.Draw({ .vertexCount = 3 });
                })
        };
        return tasks;
    }
} // namespace VisualTests