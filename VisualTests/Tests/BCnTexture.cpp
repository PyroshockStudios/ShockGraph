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

#include "BCnTexture.hpp"
#include <VisualTests/DDSLoader.hpp>

namespace VisualTests {
    void BCnTexture::CreateResources(const CreateResourceInfo& info) {
        auto ddsNPOT = LoadDDS("resources/VisualTests/Textures/NPOT_Test_BC1.dds");
        auto ddsMips = LoadDDS("resources/VisualTests/Textures/NPOT_Mip_Test_BC3.dds");

        ddsNPOT.info.usage = ImageUsageFlagBits::SHADER_RESOURCE;
        ddsNPOT.info.name = "NPOT BCn Texture";

        ddsMips.info.usage = ImageUsageFlagBits::SHADER_RESOURCE;
        ddsMips.info.name = "Mip BCn Texture";

        texNPOT = info.resourceManager.CreatePersistentImage(ddsNPOT.info, ddsNPOT.subresources);
        texMips = info.resourceManager.CreatePersistentImage(ddsMips.info, ddsMips.subresources);

        viewNPOT = info.resourceManager.DefaultShaderResourceView(texNPOT);
        viewMips = info.resourceManager.DefaultShaderResourceView(texMips);
        sampler = info.resourceManager.CreateSampler({ .name = "BCn Sampler" });

        image = info.resourceManager.CreatePersistentImage({
            .format = Format::RGBA8Unorm,
            .size = { info.displayInfo.width, info.displayInfo.height },
            .usage = ImageUsageFlagBits::RENDER_TARGET | ImageUsageFlagBits::TRANSFER_SRC | ImageUsageFlagBits::BLIT_SRC,
            .name = "BCn Test Render Image",
        });

        target = info.resourceManager.CreateColorTarget({ .image = image, .name = "Stress Test RT" });

        vsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/BCnTexture.slang",
            { .stage = ShaderStage::Vertex, .entryPoint = "vertexMain" });
        fsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/BCnTexture.slang",
            { .stage = ShaderStage::Fragment, .entryPoint = "fragmentMain" });

        pipeline = info.resourceManager.CreateRasterPipeline(
            { .colorTargetStates = { { .format = image->Info().format } } },
            {
                .vertexShaderInfo = { TaskShaderInfo{ .program = vsh } },
                .fragmentShaderInfo = { TaskShaderInfo{ .program = fsh } },
            });
    }
    void BCnTexture::ReleaseResources(const ReleaseResourceInfo& info) {
        image = {};
        texNPOT = {};
        texMips = {};
        info.resourceManager.ReleaseShaderResourceView(viewNPOT);
        info.resourceManager.ReleaseShaderResourceView(viewMips);
        info.resourceManager.ReleaseSampler(sampler);
        target = {};
        vsh = {};
        fsh = {};
        pipeline = {};
    }
    eastl::span<GenericTask*> BCnTexture::CreateTasks() {
        tasks = {
            new GraphicsCallbackTask(
                { .name = "BCn Texture", .color = LabelColor::GREEN },
                [this](GraphicsTask& task) {
                    task.BindColorTarget({ .target = target, .clear = { { 0.1f, 0.1f, 0.2f, 1.0f } } });
                },
                [this](TaskCommandList& commands) {
                    commands.SetRasterPipeline(pipeline);
                    struct Push {
                        u32 Texture;
                        u32 Sampler;
                        float OffX;
                        float OffY;
                        float SizeX;
                        float SizeY;
                    };
                    commands.PushConstant(Push{
                        .Texture = viewNPOT.index,
                        .Sampler = sampler.index,
                        .OffX = -0.8f,
                        .OffY = -0.8f,
                        .SizeX = 0.25f,
                        .SizeY = 0.25f,
                    });
                    commands.Draw({ .vertexCount = 6 });

                    commands.PushConstant(Push{
                        .Texture = viewMips.index,
                        .Sampler = sampler.index,
                        .OffX = -0.1f,
                        .OffY = 0.0f,
                        .SizeX = 0.5f,
                        .SizeY = 0.25f,
                    });
                    commands.Draw({ .vertexCount = 6 });
                })
        };
        return tasks;
    }
    bool BCnTexture::TaskSupported(IDevice* device) {
        return device->Features().bBCnTextureCompression;
    }
} // namespace VisualTests