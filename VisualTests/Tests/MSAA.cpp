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

#include "MSAA.hpp"

#include "PyroRHI/Api/IDevice.hpp"

namespace VisualTests {
    void MSAA::CreateResources(const CreateResourceInfo& info) {
        RasterizationSamples availableSampleCounts = info.resourceManager.GetInternalDevice()->Properties().msaaSupportColorTarget;
        RasterizationSamples sampleCount = RasterizationSamples::e1;
        while (availableSampleCounts != RasterizationSamples::e1) {
            reinterpret_cast<u32&>(availableSampleCounts) >>= 1;
            reinterpret_cast<u32&>(sampleCount) <<= 1;
        }

        image = info.resourceManager.CreatePersistentImage({
            .format = Format::RGBA8Unorm,
            .size = { info.displayInfo.width, info.displayInfo.height },
            .usage = ImageUsageFlagBits::RENDER_TARGET | ImageUsageFlagBits::TRANSFER_SRC | ImageUsageFlagBits::BLIT_SRC,
            .name = "MSAA Resolve Render Image",
        });
        imageMSAA = info.resourceManager.CreatePersistentImage({
            .format = Format::RGBA8Unorm,
            .size = { info.displayInfo.width, info.displayInfo.height },
            .sampleCount = sampleCount,
            .usage = ImageUsageFlagBits::RENDER_TARGET,
            .name = "MSAA Render Image",
        });
        target = info.resourceManager.CreateColorTarget({
            .image = image,
            .name = "MSAA Resolve RT",
        });
        targetMSAA = info.resourceManager.CreateColorTarget({
            .image = imageMSAA,
            .name = "MSAA RT",
        });
        vsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/MSAA.slang",
            { .stage = ShaderStage::Vertex, .entryPoint = "vertexMain", .name = "MSAA Vsh" });
        fsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/MSAA.slang",
            { .stage = ShaderStage::Fragment, .entryPoint = "fragmentMain", .name = "MSAA Fsh" });
        pipeline = info.resourceManager.CreateRasterPipeline(
            {
                .colorTargetStates = { { .format = image->Info().format } },
                .name = "Raster Pipeline",
            },
            {
                .vertexShaderInfo = { TaskShaderInfo{ .program = vsh } },
                .fragmentShaderInfo = { TaskShaderInfo{ .program = fsh } },
            });
        pipelineMSAA = info.resourceManager.CreateRasterPipeline(
            {
                .colorTargetStates = { { .format = image->Info().format } },
                .multiSampleState = {
                    .sampleCount = sampleCount,
                },
                .name = "MSAA Raster Pipeline",
            },
            {
                .vertexShaderInfo = { TaskShaderInfo{ .program = vsh } },
                .fragmentShaderInfo = { TaskShaderInfo{ .program = fsh } },
            });
    }
    void MSAA::ReleaseResources(const ReleaseResourceInfo& info) {
        image = {};
        imageMSAA = {};
        target = {};
        targetMSAA = {};
        vsh = {}; fsh = {};
        pipeline = {};
        pipelineMSAA = {};
    }
    eastl::span<GenericTask*> MSAA::CreateTasks() {
        tasks = {
            new GraphicsCallbackTask(
                { .name = "MSAA", .color = LabelColor::GREEN },
                [this](GraphicsTask& task) {
                    task.BindColorTarget({
                        .target = targetMSAA,
                        .clear = { { 0.0f, 0.0f, 0.0f, 1.0f } },
                        .resolve = target,
                    });
                },
                [this](TaskCommandList& commands) {
                    commands.SetViewport({
                        .x = 0.0f,
                        .y = static_cast<f32>(image->Info().size.height / 4),
                        .width = static_cast<f32>(image->Info().size.width / 2),
                        .height = static_cast<f32>(image->Info().size.height / 2),
                    });
                    commands.SetScissor({
                        .x = 0,
                        .y = static_cast<i32>(image->Info().size.height / 4),
                        .width = static_cast<i32>(image->Info().size.width / 2),
                        .height = static_cast<i32>(image->Info().size.height / 2),
                    });
                    commands.SetRasterPipeline(pipelineMSAA);
                    commands.Draw({ .vertexCount = 3 });
                }),
            new GraphicsCallbackTask(
                { .name = "Non MSAA", .color = LabelColor::GREEN },
                [this](GraphicsTask& task) {
                    task.BindColorTarget({
                        .target = target,
                    });
                },
                [this](TaskCommandList& commands) {
                    commands.SetViewport({
                        .x = static_cast<f32>(image->Info().size.width / 2),
                        .y = static_cast<f32>(image->Info().size.height / 4),
                        .width = static_cast<f32>(image->Info().size.width / 2),
                        .height = static_cast<f32>(image->Info().size.height / 2),
                    });
                    commands.SetScissor({
                        .x = static_cast<i32>(image->Info().size.width / 2),
                        .y = static_cast<i32>(image->Info().size.height / 4),
                        .width = static_cast<i32>(image->Info().size.width / 2),
                        .height = static_cast<i32>(image->Info().size.height / 2),
                    });
                    commands.SetRasterPipeline(pipeline);
                    commands.Draw({ .vertexCount = 3 });
                })
        };
        return tasks;
    }
    bool MSAA::TaskSupported(IDevice* device) { return device->Properties().msaaSupportColorTarget > RasterizationSamples::e1; }
} // namespace VisualTests