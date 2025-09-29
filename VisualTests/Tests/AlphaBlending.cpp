#include "AlphaBlending.hpp"

namespace VisualTests {
    static constexpr u32 NUM_OVERLAYS = 8;
    void AlphaBlending::CreateResources(const CreateResourceInfo& info) {
        image = info.resourceManager.CreatePersistentImage({
            .format = Format::RGBA8Unorm,
            .size = { info.displayInfo.width, info.displayInfo.height },
            .usage = ImageUsageFlagBits::RENDER_TARGET | ImageUsageFlagBits::TRANSFER_SRC | ImageUsageFlagBits::BLIT_SRC,
            .name = "Alpha Blending Render Image",
        });
        target = info.resourceManager.CreateColorTarget({
            .image = image,
            .name = "Alpha Blending RT",
        });
        vsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/AlphaBlending.slang",
            { .stage = ShaderStage::Vertex, .entryPoint = "vertexMain", .name = "Alpha Blending Vsh" });
        fsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/AlphaBlending.slang",
            { .stage = ShaderStage::Fragment, .entryPoint = "fragmentMain", .name = "Alpha Blending Fsh" });
        pipeline = info.resourceManager.CreateRasterPipeline(
            {
                .colorTargetStates = {
                    {
                        .format = image->Info().format,
                        .blend = BlendInfo{
                            .colorBlendOp = BlendOp::Add,
                            .srcColorBlendFactor = BlendFactor::SrcAlpha,
                            .dstColorBlendFactor = BlendFactor::OneMinusSrcAlpha,
                            .alphaBlendOp = BlendOp::Add,
                            .srcAlphaBlendFactor = BlendFactor::Zero,
                            .dstAlphaBlendFactor = BlendFactor::One },
                    } },
                .name = "Raster Pipeline",
            },
            {
                .vertexShaderInfo = { TaskShaderInfo{ .program = vsh } },
                .fragmentShaderInfo = { TaskShaderInfo{ .program = fsh } },
            });
    }
    void AlphaBlending::ReleaseResources(const ReleaseResourceInfo& info) {
        image = {};
        target = {};
        vsh, fsh = {};
        pipeline = {};
    }
    eastl::span<GenericTask*> AlphaBlending::CreateTasks() {
        tasks = {
            new GraphicsCallbackTask(
                { .name = "Alpha Blending", .color = LabelColor::GREEN },
                [this](GraphicsTask& task) {
                    task.BindColorTarget({
                        .target = target,
                        .clear = { { 0.0f, 0.0f, 0.0f, 1.0f } },
                        .bBlending = true,
                    });
                },
                [this](TaskCommandList& commands) {
                    commands.SetRasterPipeline(pipeline);
                    commands.Draw({ .vertexCount = 3, .instanceCount = NUM_OVERLAYS });
                })
        };
        return tasks;
    }
} // namespace VisualTests