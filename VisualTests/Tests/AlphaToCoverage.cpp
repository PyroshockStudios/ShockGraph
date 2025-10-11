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

#include "AlphaToCoverage.hpp"

#include "PyroRHI/Api/IDevice.hpp"

namespace VisualTests {
    static const eastl::array<f32, 2> gOffset0 = { -0.5f, 0.0f };
    static const eastl::array<f32, 2> gOffset1 = { 0.5, 0.0f };

    void AlphaToCoverage::CreateResources(const CreateResourceInfo& info) {
        RasterizationSamples sampleCount = info.resourceManager.GetInternalDevice()->GetProperties().maxRenderTargetSamples;
        image = info.resourceManager.CreatePersistentImage({
            .format = Format::RGBA8Unorm,
            .size = { info.displayInfo.width, info.displayInfo.height },
            .usage = ImageUsageFlagBits::RENDER_TARGET | ImageUsageFlagBits::TRANSFER_SRC | ImageUsageFlagBits::BLIT_SRC,
            .name = "AlphaToCoverage Resolve Render Image",
        });
        imageMSAA = info.resourceManager.CreatePersistentImage({
            .format = Format::RGBA8Unorm,
            .size = { info.displayInfo.width, info.displayInfo.height },
            .sampleCount = sampleCount,
            .usage = ImageUsageFlagBits::RENDER_TARGET,
            .name = "AlphaToCoverage MSAA Render Image",
        });
        target = info.resourceManager.CreateColorTarget({
            .image = image,
            .name = "AlphaToCoverage Resolve RT",
        });
        targetMSAA = info.resourceManager.CreateColorTarget({
            .image = imageMSAA,
            .name = "AlphaToCoverage RT",
        });
        vsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/AlphaToCoverage.slang",
            { .stage = ShaderStage::Vertex, .entryPoint = "vertexMain", .name = "AlphaToCoverage Vsh" });
        fsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/AlphaToCoverage.slang",
            { .stage = ShaderStage::Fragment, .entryPoint = "fragmentMain", .name = "AlphaToCoverage Fsh" });
        pipeline = info.resourceManager.CreateRasterPipeline(
            {
                .colorTargetStates = { { .format = image->Info().format } },
                .multiSampleState = {
                    .sampleCount = sampleCount,
                    .bAlphaToCoverage = true,
                },
                .name = "Raster Pipeline",
            },
            {
                .vertexShaderInfo = { TaskShaderInfo{ .program = vsh } },
                .fragmentShaderInfo = { TaskShaderInfo{ .program = fsh } },
            });
    }
    void AlphaToCoverage::ReleaseResources(const ReleaseResourceInfo& info) {
        image = {};
        imageMSAA = {};
        target = {};
        targetMSAA = {};
        vsh, fsh = {};
        pipeline = {};
    }
    eastl::span<GenericTask*> AlphaToCoverage::CreateTasks() {
        tasks = {
            new GraphicsCallbackTask(
                { .name = "AlphaToCoverage", .color = LabelColor::GREEN },
                [this](GraphicsTask& task) {
                    task.BindColorTarget({
                        .target = targetMSAA,
                        .clear = { { 0.0f, 0.0f, 0.0f, 1.0f } },
                        .resolve = target,
                    });
                },
                [this](TaskCommandList& commands) {
                    commands.SetRasterPipeline(pipeline);
                    commands.PushConstant(gOffset0);
                    commands.Draw({ .vertexCount = 3 });
                })
        };
        return tasks;
    }
} // namespace VisualTests