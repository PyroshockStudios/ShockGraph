#include "Wireframe.hpp"

namespace VisualTests {
    void Wireframe::CreateResources(const CreateResourceInfo& info) {
        image = info.resourceManager.CreatePersistentImage({
            .format = Format::RGBA8Unorm,
            .size = { info.displayInfo.width, info.displayInfo.height },
            .usage = ImageUsageFlagBits::RENDER_TARGET | ImageUsageFlagBits::TRANSFER_SRC | ImageUsageFlagBits::BLIT_SRC,
            .name = "Wireframe Render Image",
        });
        target = info.resourceManager.CreateColorTarget({
            .image = image,
            .name = "Wireframe RT",
        });
        vsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/Wireframe.slang",
            { .stage = ShaderStage::Vertex, .entryPoint = "vertexMain", .name = "Wireframe Vsh" });
        fsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/Wireframe.slang",
            { .stage = ShaderStage::Fragment, .entryPoint = "fragmentMain", .name = "Wireframe Fsh" });
        pipeline = info.resourceManager.CreateRasterPipeline(
            {
                .colorTargetStates = { { .format = image->Info().format } },
                .rasterizerState = { .polygonMode = PolygonMode::Line },
                .name = "Raster Pipeline",
            },
            {
                .vertexShaderInfo = { TaskShaderInfo{ .program = vsh } },
                .fragmentShaderInfo = { TaskShaderInfo{ .program = fsh } },
            });
    }
    void Wireframe::ReleaseResources(const ReleaseResourceInfo& info) {
        image = {};
        target = {};
        vsh, fsh = {};
        pipeline = {};
    }
    eastl::span<GenericTask*> Wireframe::CreateTasks() {
        tasks = {
            new GraphicsCallbackTask(
                { .name = "Wireframe", .color = LabelColor::GREEN },
                [this](GraphicsTask& task) {
                    task.BindColorTarget({
                        .target = target,
                        .clear = { { 0.0f, 0.0f, 0.0f, 1.0f } },
                    });
                },
                [this](TaskCommandList& commands) {
                    commands.SetRasterPipeline(pipeline);
                    commands.Draw({ .vertexCount = 54 });
                })
        };
        return tasks;
    }
} // namespace VisualTests