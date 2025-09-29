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

#include "BlitImage.hpp"

namespace VisualTests {
    void BlitImage::CreateResources(const CreateResourceInfo& info) {
        image = info.resourceManager.CreatePersistentImage({
            .format = Format::RGBA32Sfloat /*also try out different formats!*/,
            // render to much smaller texture
            .size = { info.displayInfo.width / 8, info.displayInfo.height / 8  },
            .usage = ImageUsageFlagBits::RENDER_TARGET | ImageUsageFlagBits::BLIT_SRC,
            .name = "Blit Image Render Image",
        });
        blitImage = info.resourceManager.CreatePersistentImage({
            .format = Format::A2RGB10Unorm /*also try out different formats!*/,
            .size = { info.displayInfo.width, info.displayInfo.height },
            .usage = ImageUsageFlagBits::TRANSFER_SRC | ImageUsageFlagBits::BLIT_DST | ImageUsageFlagBits::BLIT_SRC,
            .name = "Blit Image Blit Image",
        });
        target = info.resourceManager.CreateColorTarget({
            .image = image,
            .name = "Blit Image RT",
        });
        vsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/BlitImage.slang",
            { .stage = ShaderStage::Vertex, .entryPoint = "vertexMain", .name = "Blit Image Vsh" });
        fsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/BlitImage.slang",
            { .stage = ShaderStage::Fragment, .entryPoint = "fragmentMain", .name = "Blit Image Fsh" });
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
    void BlitImage::ReleaseResources(const ReleaseResourceInfo& info) {
        image = {};
        blitImage = {};
        target = {};
        vsh, fsh = {};
        pipeline = {};
    }
    eastl::span<GenericTask*> BlitImage::CreateTasks() {
        tasks = {
            new GraphicsCallbackTask(
                { .name = "Blit Image Triangle", .color = LabelColor::GREEN },
                [this](GraphicsTask& task) {
                    task.BindColorTarget({
                        .target = target,
                        .clear = { { 0.0f, 0.0f, 0.0f, 1.0f } },
                    });
                },
                [this](TaskCommandList& commands) {
                    commands.SetRasterPipeline(pipeline);
                    commands.Draw({ .vertexCount = 3 });
                }),
            new CustomCallbackTask(
                { .name = "Blit Image Operations", .color = LabelColor::GREEN },
                [this](CustomTask& task) {
                    task.UseImage({ .image = image,
                        .access = AccessConsts::BLIT_READ });
                    task.UseImage({ .image = blitImage,
                        .access = AccessConsts::BLIT_WRITE });
                },
                [this](ICommandBuffer* commands) {
                    BlitImageToImageInfo blitInfo{};
                    blitInfo.srcImage = image->Internal();
                    blitInfo.dstImage = blitImage->Internal();

                    Extent3D srcDim = image->Info().size;
                    Extent3D dstDim = blitImage->Info().size;

                    // Destination layout: 2x2 grid in dst image
                    // Each quadrant will test a different blit configuration
                    i32 halfW = dstDim.x / 2;
                    i32 halfH = dstDim.y / 2;

                    // Configurations to test
                    struct TestConfig {
                        Rect2D srcRect;
                        Rect2D dstRect;
                        Filter filter;
                    };

                    eastl::vector<TestConfig> tests;

                    // 0: "Clear" blit. Blit from empty src to full dst
                    tests.push_back({ { 0, 0, 1, 1 },
                        { 0, 0, (i32)dstDim.x, (i32)dstDim.y },
                        Filter::Nearest });

                    // 1: Full source -> top-left quadrant, nearest filter
                    tests.push_back({ { 0, 0, (i32)srcDim.x, (i32)srcDim.y },
                        { 0, halfH, halfW, halfH },
                        Filter::Nearest });

                    // 2: Center quarter of source -> top-right quadrant, linear filter
                    tests.push_back({ { (i32)srcDim.x / 4, (i32)srcDim.y / 4,
                                          (i32)srcDim.x / 2, (i32)srcDim.y / 2 },
                        { halfW, halfH, halfW, halfH },
                        Filter::Linear });

                    // 3: Flipped X source -> bottom-left quadrant, nearest filter
                    tests.push_back({ { (i32)srcDim.x, 0, -(i32)srcDim.x, (i32)srcDim.y },
                        { 0, 0, halfW, halfH },
                        Filter::Nearest });

                    // 4: Flipped Y + scaled -> bottom-right quadrant, linear filter
                    tests.push_back({ { 0, (i32)srcDim.y, (i32)srcDim.x, -(i32)srcDim.y },
                        { halfW + halfW / 4, halfH / 4, halfW / 2, halfH / 2 },
                        Filter::Linear });

                    // Now run all blits
                    for (auto& t : tests) {
                        blitInfo.srcImageRect = t.srcRect;
                        blitInfo.dstImageRect = t.dstRect;
                        blitInfo.filter = t.filter;

                        commands->BlitImageToImage(blitInfo);
                    }
                },
                TaskType::Graphics)
        };
        return tasks;
    }
} // namespace VisualTests