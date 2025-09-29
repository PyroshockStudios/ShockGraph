#include "MSAA.hpp"

#include "PyroRHI/Api/IDevice.hpp"

namespace VisualTests {
    void MSAA::CreateResources(const CreateResourceInfo& info) {
        RasterizationSamples sampleCount = info.resourceManager.GetInternalDevice()->GetLimits().maxRenderTargetSamples;
        image = info.resourceManager.CreatePersistentImage({
            .format = Format::RGBA8Unorm,
            .size = { info.displayInfo.width, info.displayInfo.height },
            .usage = ImageUsageFlagBits::RENDER_TARGET | ImageUsageFlagBits::TRANSFER_SRC | ImageUsageFlagBits::BLIT_SRC,
            .name = "MSAA Resolve Render Image",
        });
        imageMSAA = info.resourceManager.CreatePersistentImage({
            .format = Format::RGBA8Unorm,
            .size = { info.displayInfo.width, info.displayInfo.height },
            .sampleCount = static_cast<u32>(sampleCount),
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
        vsh, fsh = {};
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
                        .y = static_cast<f32>(image->Info().size.y / 4),
                        .width = static_cast<f32>(image->Info().size.x / 2),
                        .height = static_cast<f32>(image->Info().size.y / 2),
                    });
                    commands.SetScissor({
                        .x = 0,
                        .y = static_cast<i32>(image->Info().size.y / 4),
                        .width = static_cast<i32>(image->Info().size.x / 2),
                        .height = static_cast<i32>(image->Info().size.y / 2),
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
                        .x = static_cast<f32>(image->Info().size.x / 2),
                        .y = static_cast<f32>(image->Info().size.y / 4),
                        .width = static_cast<f32>(image->Info().size.x / 2),
                        .height = static_cast<f32>(image->Info().size.y / 2),
                    });
                    commands.SetScissor({
                        .x = static_cast<i32>(image->Info().size.x / 2),
                        .y = static_cast<i32>(image->Info().size.y / 4),
                        .width = static_cast<i32>(image->Info().size.x / 2),
                        .height = static_cast<i32>(image->Info().size.y / 2),
                    });
                    commands.SetRasterPipeline(pipeline);
                    commands.Draw({ .vertexCount = 3 });
                })
        };
        return tasks;
    }
} // namespace VisualTests