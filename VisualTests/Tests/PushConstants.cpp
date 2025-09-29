#include "PushConstants.hpp"

namespace VisualTests {
    void PushConstants::CreateResources(const CreateResourceInfo& info) {
        image = info.resourceManager.CreatePersistentImage({
            .format = Format::RGBA8Unorm,
            .size = { info.displayInfo.width, info.displayInfo.height },
            .usage = ImageUsageFlagBits::RENDER_TARGET | ImageUsageFlagBits::TRANSFER_SRC | ImageUsageFlagBits::BLIT_SRC,
            .name = "Push Constants Render Image",
        });
        target = info.resourceManager.CreateColorTarget({
            .image = image,
            .name = "Push Constants RT",
        });
        vsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/PushConstants.slang",
            { .stage = ShaderStage::Vertex, .entryPoint = "vertexMain", .name = "Push Constants Vsh" });
        fsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/PushConstants.slang",
            { .stage = ShaderStage::Fragment, .entryPoint = "fragmentMain", .name = "Push Constants Fsh" });
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
    void PushConstants::ReleaseResources(const ReleaseResourceInfo& info) {
        image = {};
        target = {};
        vsh, fsh = {};
        pipeline = {};
    }
    eastl::span<GenericTask*> PushConstants::CreateTasks() {
        tasks = {
            new GraphicsCallbackTask(
                { .name = "Push Constants", .color = LabelColor::GREEN },
                [this](GraphicsTask& task) {
                    task.BindColorTarget({
                        .target = target,
                        .clear = { { 0.0f, 0.0f, 0.0f, 1.0f } },
                    });
                },
                [this](TaskCommandList& commands) {
                    static float rotation = 0.0f;
                    commands.SetRasterPipeline(pipeline);
                    commands.PushConstant(rotation);
                    commands.Draw({ .vertexCount = 3 });
                    rotation += 1.0f / 200.0f;
                })
        };
        return tasks;
    }
} // namespace VisualTests