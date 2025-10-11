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

#include "HelloTexture.hpp"

namespace VisualTests {
    void HelloTexture::CreateResources(const CreateResourceInfo& info) {
        eastl::vector<u8> textureData(8 * 8 * 4); // RGBA8

        for (int y = 0; y < 8; ++y) {
            for (int x = 0; x < 8; ++x) {
                int idx = (y * 8 + x) * 4;

                // Checkerboard: alternate every 4 pixels
                bool isCol = ((x / 4) % 2) == ((y / 4) % 2);

                textureData[idx + 0] = isCol ? 255 : 0; // R
                textureData[idx + 1] = 0;               // G
                textureData[idx + 2] = isCol ? 255 : 0; // B
                textureData[idx + 3] = 255;             // A
            }
        }

        image = info.resourceManager.CreatePersistentImage({
            .format = Format::RGBA8Unorm,
            .size = { info.displayInfo.width, info.displayInfo.height },
            .usage = ImageUsageFlagBits::RENDER_TARGET | ImageUsageFlagBits::TRANSFER_SRC | ImageUsageFlagBits::BLIT_SRC,
            .name = "Hello Texture Render Image",
        });
        texture = info.resourceManager.CreatePersistentImage(
            {
                .format = Format::RGBA8Unorm,
                .size = { 8, 8 },
                .usage = ImageUsageFlagBits::SHADER_RESOURCE,
                .name = "Hello Texture Input",
            },
            { textureData.cbegin(), textureData.cend() });
        textureView = info.resourceManager.DefaultShaderResourceView(texture);
        sampler = info.resourceManager.CreateSampler({ .name = "Hello Texture Sampler" });

        target = info.resourceManager.CreateColorTarget({
            .image = image,
            .name = "Hello Texture RT",
        });
        vsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/HelloTexture.slang",
            { .stage = ShaderStage::Vertex, .entryPoint = "vertexMain", .name = "Hello Texture Vsh" });
        fsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/HelloTexture.slang",
            { .stage = ShaderStage::Fragment, .entryPoint = "fragmentMain", .name = "Hello Texture Fsh" });
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
    void HelloTexture::ReleaseResources(const ReleaseResourceInfo& info) {
        image = {};
        info.resourceManager.ReleaseShaderResourceView(textureView);
        texture = {};
        info.resourceManager.ReleaseSampler(sampler);
        target = {};
        vsh = {}; fsh = {};
        pipeline = {};
    }
    eastl::span<GenericTask*> HelloTexture::CreateTasks() {
        tasks = {
            new GraphicsCallbackTask(
                { .name = "Hello Texture", .color = LabelColor::GREEN },
                [this](GraphicsTask& task) {
                    task.BindColorTarget({
                        .target = target,
                        .clear = { { 0.0f, 0.0f, 0.0f, 1.0f } },
                    });
                },
                [this](TaskCommandList& commands) {
                    commands.SetRasterPipeline(pipeline);
                    struct Push {
                        u32 Texture;
                        u32 Sampler;
                    };
                    commands.PushConstant<Push>({
                        .Texture = textureView.index,
                        .Sampler = sampler.index,
                    });
                    commands.Draw({ .vertexCount = 6 });
                })
        };
        return tasks;
    }
} // namespace VisualTests