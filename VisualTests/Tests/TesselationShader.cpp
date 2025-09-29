#include "TesselationShader.hpp"

namespace VisualTests {
    void TesselationShader::CreateResources(const CreateResourceInfo& info) {
        image = info.resourceManager.CreatePersistentImage({
            .format = Format::RGBA8Unorm,
            .size = { info.displayInfo.width, info.displayInfo.height },
            .usage = ImageUsageFlagBits::RENDER_TARGET | ImageUsageFlagBits::TRANSFER_SRC | ImageUsageFlagBits::BLIT_SRC,
            .name = "Tesselation Shader Render Image",
        });
        target = info.resourceManager.CreateColorTarget({
            .image = image,
            .name = "Tesselation Shader RT",
        });
        vsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/TesselationShader.slang",
            { .stage = ShaderStage::Vertex, .entryPoint = "vertexMain", .name = "Tesselation Shader Vsh" });
        hsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/TesselationShader.slang",
            { .stage = ShaderStage::Hull, .entryPoint = "hullMain", .name = "Tesselation Shader Hsh" });
        dsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/TesselationShader.slang",
            { .stage = ShaderStage::Domain, .entryPoint = "domainMain", .name = "Tesselation Shader Dsh" });
        fsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/TesselationShader.slang",
            { .stage = ShaderStage::Fragment, .entryPoint = "fragmentMain", .name = "Tesselation Shader Fsh" });
        pipeline = info.resourceManager.CreateRasterPipeline(
            {
                .colorTargetStates = { { .format = image->Info().format } },
                .tesselationState = { { .controlPoints = 3 } },
                .inputAssemblyState = { .primitiveTopology = PrimitiveTopology::PatchList },
                .rasterizerState = { .polygonMode = PolygonMode::Line },
                .name = "Raster Pipeline",
            },
            {
                .vertexShaderInfo = { TaskShaderInfo{ .program = vsh } },
                .hullShaderInfo = { TaskShaderInfo{ .program = hsh } },
                .domainShaderInfo = { TaskShaderInfo{ .program = dsh } },
                .fragmentShaderInfo = { TaskShaderInfo{ .program = fsh } },
            });
    }
    void TesselationShader::ReleaseResources(const ReleaseResourceInfo& info) {
        image = {};
        target = {};
        vsh, fsh = {};
        pipeline = {};
    }
    eastl::span<GenericTask*> TesselationShader::CreateTasks() {
        tasks = {
            new GraphicsCallbackTask(
                { .name = "Tesselation Shader", .color = LabelColor::GREEN },
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