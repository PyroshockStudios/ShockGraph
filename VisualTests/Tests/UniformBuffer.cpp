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

#include "UniformBuffer.hpp"

namespace VisualTests {

    using f32 = float;
    using Vec4 = std::array<f32, 4>;
    using ColorArray3 = std::array<Vec4, 3>;

    struct GlobalUbo {
        ColorArray3 colors;
    };

    // Converts HSV [0..1] -> RGB [0..1]
    Vec4 HSVtoRGB(f32 h, f32 s, f32 v, f32 a = 1.0f) {
        h = fmodf(h, 1.0f);
        f32 c = v * s;
        f32 x = c * (1.0f - fabsf(fmodf(h * 6.0f, 2.0f) - 1.0f));
        f32 m = v - c;
        f32 r, g, b;

        if (h < 1.0f / 6.0f) {
            r = c;
            g = x;
            b = 0;
        } else if (h < 2.0f / 6.0f) {
            r = x;
            g = c;
            b = 0;
        } else if (h < 3.0f / 6.0f) {
            r = 0;
            g = c;
            b = x;
        } else if (h < 4.0f / 6.0f) {
            r = 0;
            g = x;
            b = c;
        } else if (h < 5.0f / 6.0f) {
            r = x;
            g = 0;
            b = c;
        } else {
            r = c;
            g = 0;
            b = x;
        }

        return { r + m, g + m, b + m, a };
    }

    // Generates three colors with hue rotated evenly around the circle
    ColorArray3 HueRotateThree(f32 rotation) {
        ColorArray3 result;
        for (int i = 0; i < 3; ++i) {
            // evenly spaced hues: 0, 1/3, 2/3
            f32 hue = fmodf(rotation + static_cast<float>(i) / 3.0f, 1.0f);
            result[i] = HSVtoRGB(hue, 1.0f, 1.0f); // full saturation & value
        }
        return result;
    }

    void UniformBuffer::CreateResources(const CreateResourceInfo& info) {
        image = info.resourceManager.CreatePersistentImage({
            .format = Format::RGBA8Unorm,
            .size = { info.displayInfo.width, info.displayInfo.height },
            .usage = ImageUsageFlagBits::RENDER_TARGET | ImageUsageFlagBits::TRANSFER_SRC | ImageUsageFlagBits::BLIT_SRC,
            .name = "Uniform Buffer Render Image",
        });
        ubo = info.resourceManager.CreatePersistentBuffer({
            .size = sizeof(GlobalUbo),
            .usage = BufferUsageFlagBits::UNIFORM_BUFFER,
            .mode = TaskBufferMode::Dynamic,
            .name = "Vertex Colours Uniform Buffer",
        });
        target = info.resourceManager.CreateColorTarget({
            .image = image,
            .name = "Uniform Buffer RT",
        });
        vsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/UniformBuffer.slang",
            { .stage = ShaderStage::Vertex, .entryPoint = "vertexMain", .name = "Uniform Buffer Vsh" });
        fsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/UniformBuffer.slang",
            { .stage = ShaderStage::Fragment, .entryPoint = "fragmentMain", .name = "Uniform Buffer Fsh" });
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
    void UniformBuffer::ReleaseResources(const ReleaseResourceInfo& info) {
        image = {};
        ubo = {};
        target = {};
        vsh = {}; fsh = {};
        pipeline = {};
    }
    eastl::span<GenericTask*> UniformBuffer::CreateTasks() {
        tasks = {
            new GraphicsCallbackTask(
                { .name = "Uniform Buffer", .color = LabelColor::GREEN },
                [this](GraphicsTask& task) {
                    task.BindColorTarget({
                        .target = target,
                        .clear = { { 0.0f, 0.0f, 0.0f, 1.0f } },
                    });
                },
                [this](TaskCommandList& commands) {
                    static float rotation = 0.0f;
                    auto* uboData = reinterpret_cast<GlobalUbo*>(ubo->MappedMemory());
                    uboData->colors = HueRotateThree(rotation);

                    commands.SetRasterPipeline(pipeline);
                    commands.SetUniformBufferView({
                        .slot = 0,
                        .buffer = ubo,
                    });

                    commands.Draw({ .vertexCount = 3 });
                    rotation += 1.0f / 200.0f;
                    if (rotation > 1.0f)
                        rotation -= 1.0f;
                })
        };
        return tasks;
    }
} // namespace VisualTests