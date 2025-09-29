#include "GeometryShader.hpp"

namespace VisualTests {
    void GeometryShader::CreateResources(const CreateResourceInfo& info) {
        image = info.resourceManager.CreatePersistentImage({
            .format = Format::RGBA8Unorm,
            .size = { info.displayInfo.width, info.displayInfo.height },
            .usage = ImageUsageFlagBits::RENDER_TARGET | ImageUsageFlagBits::TRANSFER_SRC | ImageUsageFlagBits::BLIT_SRC,
            .name = "Geometry Shader Render Image",
        });
        target = info.resourceManager.CreateColorTarget({
            .image = image,
            .name = "Geometry Shader RT",
        });
        vsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/GeometryShader.slang",
            { .stage = ShaderStage::Vertex, .entryPoint = "vertexMain", .name = "Geometry Shader Vsh" });
        gsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/GeometryShader.slang",
            { .stage = ShaderStage::Geometry, .entryPoint = "geometryMain", .name = "Geometry Shader Gsh" });
        fsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/GeometryShader.slang",
            { .stage = ShaderStage::Fragment, .entryPoint = "fragmentMain", .name = "Geometry Shader Fsh" });
        pipeline = info.resourceManager.CreateRasterPipeline(
            {
                .colorTargetStates = { { .format = image->Info().format } },
                .name = "Raster Pipeline",
            },
            {
                .vertexShaderInfo = { TaskShaderInfo{ .program = vsh } },
                .geometryShaderInfo = { TaskShaderInfo{ .program = gsh } },
                .fragmentShaderInfo = { TaskShaderInfo{ .program = fsh } },
            });
    }
    void GeometryShader::ReleaseResources(const ReleaseResourceInfo& info) {
        image = {};
        target = {};
        vsh, gsh, fsh = {};
        pipeline = {};
    }
    eastl::span<GenericTask*> GeometryShader::CreateTasks() {
        tasks = {
            new GraphicsCallbackTask(
                { .name = "Geometry Shader", .color = LabelColor::GREEN },
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