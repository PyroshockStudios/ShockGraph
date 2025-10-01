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

#include "GeometryShader.hpp"
#include <PyroRHI/Api/IDevice.hpp>
#include <PyroRHI/Context.hpp>

namespace VisualTests {
    void GeometryShader::CreateResources(const CreateResourceInfo& info) {
        image = info.resourceManager.CreatePersistentImage({
            .format = Format::RGBA8Unorm,
            .size = { info.displayInfo.width, info.displayInfo.height },
            .usage = ImageUsageFlagBits::RENDER_TARGET | ImageUsageFlagBits::TRANSFER_SRC | ImageUsageFlagBits::BLIT_SRC,
            .name = "Geometry Shader Render Image",
        });
        target = info.resourceManager.CreateColorTarget({
            .image = image,
            .name = "Geometry Shader RT",
        });
        vsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/GeometryShader.slang",
            { .stage = ShaderStage::Vertex, .entryPoint = "vertexMain", .name = "Geometry Shader Vsh" });
        if (info.resourceManager.GetInternalContext()->Properties().bGeometryShader) {
            gsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/GeometryShader.slang",
                { .stage = ShaderStage::Geometry, .entryPoint = "geometryMain", .name = "Geometry Shader Gsh" });
        }
        fsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/GeometryShader.slang",
            { .stage = ShaderStage::Fragment, .entryPoint = "fragmentMain", .name = "Geometry Shader Fsh" });
        pipeline = info.resourceManager.CreateRasterPipeline(
            {
                .colorTargetStates = { { .format = image->Info().format } },
                .name = "Raster Pipeline",
            },
            {
                .vertexShaderInfo = { TaskShaderInfo{ .program = vsh } },
                .geometryShaderInfo = gsh ? eastl::optional{ TaskShaderInfo{ .program = gsh } } : eastl::nullopt,
                .fragmentShaderInfo = { TaskShaderInfo{ .program = fsh } },
            });
    }
    void GeometryShader::ReleaseResources(const ReleaseResourceInfo& info) {
        image = {};
        target = {};
        vsh, gsh, fsh = {};
        pipeline = {};
    }
    eastl::span<GenericTask*> GeometryShader::CreateTasks() {
        tasks = {
            new GraphicsCallbackTask(
                { .name = "Geometry Shader", .color = LabelColor::GREEN },
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