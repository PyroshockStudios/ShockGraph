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

#include "UpdateBuffer.hpp"

namespace VisualTests {

    void UpdateBuffer::CreateResources(const CreateResourceInfo& info) {
        image = info.resourceManager.CreatePersistentImage({
            .format = Format::RGBA8Unorm,
            .size = { info.displayInfo.width, info.displayInfo.height },
            .usage = ImageUsageFlagBits::RENDER_TARGET | ImageUsageFlagBits::TRANSFER_SRC | ImageUsageFlagBits::BLIT_SRC,
            .name = "Update Buffer Render Image",
        });
        ubo = info.resourceManager.CreatePersistentBuffer({
            .size = sizeof(f32),
            .usage = BufferUsageFlagBits::UNIFORM_BUFFER | BufferUsageFlagBits::TRANSFER_DST /*Transfer DST is required*/,
            .bCpuVisible = false, // Not cpu visible! Should not be  needed for UpdateBuffer()!
            .bDynamic = false,    // Not dynamic! Should not be needed either!
            .name = "Vertex Scale Uniform Buffer",
        });
        target = info.resourceManager.CreateColorTarget({
            .image = image,
            .name = "Update Buffer RT",
        });
        vsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/UpdateBuffer.slang",
            { .stage = ShaderStage::Vertex, .entryPoint = "vertexMain", .name = "Update Buffer Vsh" });
        fsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/UpdateBuffer.slang",
            { .stage = ShaderStage::Fragment, .entryPoint = "fragmentMain", .name = "Update Buffer Fsh" });
        pipeline = info.resourceManager.CreateRasterPipeline(
            {
                .colorTargetStates = { { .format = image->Info().format } },
                .name = "Raster Pipeline",
            },
            {
                .vertexShaderInfo = { TaskShaderInfo{ .program = vsh } },
                .fragmentShaderInfo = { TaskShaderInfo{ .program = fsh } },
            });
    }
    void UpdateBuffer::ReleaseResources(const ReleaseResourceInfo& info) {
        image = {};
        ubo = {};
        target = {};
        vsh = {}; fsh = {};
        pipeline = {};
    }
    eastl::span<GenericTask*> UpdateBuffer::CreateTasks() {
        tasks = {
            // Update buffer is basically a transfer operation,
            // Access the buffer with TRANSFER_WRITE
            new TransferCallbackTask(
                { .name = "Update Buffer", .color = LabelColor::GREEN },
                [this](TransferTask& task) {
                    task.UseBuffer({ .buffer = ubo,
                        .access = AccessConsts::TRANSFER_WRITE });
                },
                [this](TaskCommandList& commands) {
                    static float time = 0.0f;
                    time += 1.0f / 200.0f;
                    float scale = sin(time) * 0.5f + 0.5f;
                    commands.UpdateBuffer({
                        .buffer = ubo,
                        .data = &scale,
                    });
                }),
            new GraphicsCallbackTask(
                { .name = "Read Buffer", .color = LabelColor::GREEN },
                [this](GraphicsTask& task) {
                    task.BindColorTarget({
                        .target = target,
                        .clear = { { 0.0f, 0.0f, 0.0f, 1.0f } },
                    });
                    // Finally, read from the buffer
                    task.UseBuffer({ .buffer = ubo,
                        .access = AccessConsts::VERTEX_SHADER_READ });
                },
                [this](TaskCommandList& commands) {
                    commands.SetRasterPipeline(pipeline);
                    commands.SetUniformBufferView({
                        .slot = 0,
                        .buffer = ubo,
                    });

                    commands.Draw({ .vertexCount = 3 });
                })
        };
        return tasks;
    }
} // namespace VisualTests