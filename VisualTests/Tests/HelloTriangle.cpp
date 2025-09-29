#include "HelloTriangle.hpp"

namespace VisualTests {
    void HelloTriangle::CreateResources(const CreateResourceInfo& info) {
        image = info.resourceManager.CreatePersistentImage({
            .format = Format::RGBA8Unorm,
            .size = { info.displayInfo.width, info.displayInfo.height },
            .usage = ImageUsageFlagBits::RENDER_TARGET | ImageUsageFlagBits::TRANSFER_SRC | ImageUsageFlagBits::BLIT_SRC,
            .name = "Hello Triangle Render Image",
        });
        target = info.resourceManager.CreateColorTarget({
            .image = image,
            .name = "Hello Triangle RT",
        });
        vsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/HelloTriangle.slang",
            { .stage = ShaderStage::Vertex, .entryPoint = "vertexMain", .name = "Hello Triangle Vsh" });
        fsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/HelloTriangle.slang",
            { .stage = ShaderStage::Fragment, .entryPoint = "fragmentMain", .name = "Hello Triangle Fsh" });
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
    void HelloTriangle::ReleaseResources(const ReleaseResourceInfo& info) {
        image = {};
        target = {};
        vsh, fsh = {};
        pipeline = {};
    }
    eastl::span<GenericTask*> HelloTriangle::CreateTasks() {
        tasks = {
            new GraphicsCallbackTask(
                { .name = "Hello Triangle", .color = LabelColor::GREEN },
                [this](GraphicsTask& task) {
                    task.BindColorTarget({
                        .target = target,
                        .clear = { { 0.0f, 0.0f, 0.0f, 1.0f } },
                    });
                },
                [this](TaskCommandList& commands) {
                    commands.SetRasterPipeline(pipeline);
                    commands.Draw({ .vertexCount = 3 });
                })
        };
        return tasks;
    }
} // namespace VisualTests