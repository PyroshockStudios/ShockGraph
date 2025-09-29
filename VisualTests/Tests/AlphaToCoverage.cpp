#include "AlphaToCoverage.hpp"

#include "PyroRHI/Api/IDevice.hpp"

namespace VisualTests {
    static const eastl::array<f32, 2> gOffset0 = { -0.5f, 0.0f };
    static const eastl::array<f32, 2> gOffset1 = { 0.5, 0.0f };

    void AlphaToCoverage::CreateResources(const CreateResourceInfo& info) {
        RasterizationSamples sampleCount = info.resourceManager.GetInternalDevice()->GetLimits().maxRenderTargetSamples;
        image = info.resourceManager.CreatePersistentImage({
            .format = Format::RGBA8Unorm,
            .size = { info.displayInfo.width, info.displayInfo.height },
            .usage = ImageUsageFlagBits::RENDER_TARGET | ImageUsageFlagBits::TRANSFER_SRC | ImageUsageFlagBits::BLIT_SRC,
            .name = "AlphaToCoverage Resolve Render Image",
        });
        imageMSAA = info.resourceManager.CreatePersistentImage({
            .format = Format::RGBA8Unorm,
            .size = { info.displayInfo.width, info.displayInfo.height },
            .sampleCount = static_cast<u32>(sampleCount),
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