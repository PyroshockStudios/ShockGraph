#include "WireframeSmooth.hpp"

namespace VisualTests {
    void WireframeSmooth::CreateResources(const CreateResourceInfo& info) {
        image = info.resourceManager.CreatePersistentImage({
            .format = Format::RGBA8Unorm,
            .size = { info.displayInfo.width, info.displayInfo.height },
            .usage = ImageUsageFlagBits::RENDER_TARGET | ImageUsageFlagBits::TRANSFER_SRC | ImageUsageFlagBits::BLIT_SRC,
            .name = "Wireframe Smooth Render Image",
        });
        target = info.resourceManager.CreateColorTarget({
            .image = image,
            .name = "Wireframe Smooth RT",
        });
        vsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/WireframeSmooth.slang",
            { .stage = ShaderStage::Vertex, .entryPoint = "vertexMain", .name = "WireframeSmooth Vsh" });
        fsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/WireframeSmooth.slang",
            { .stage = ShaderStage::Fragment, .entryPoint = "fragmentMain", .name = "WireframeSmooth Fsh" });
        pipeline = info.resourceManager.CreateRasterPipeline(
            {
                .colorTargetStates = { {
                    .format = image->Info().format,
                    // alpha blending is required for smooth lines
                    .blend = BlendInfo{
                        .colorBlendOp = BlendOp::Add,
                        .srcColorBlendFactor = BlendFactor::SrcAlpha,
                        .dstColorBlendFactor = BlendFactor::OneMinusSrcAlpha,
                        .alphaBlendOp = BlendOp::Add,
                        .srcAlphaBlendFactor = BlendFactor::Zero,
                        .dstAlphaBlendFactor = BlendFactor::One },
                } },
                .rasterizerState = {
                    .polygonMode = PolygonMode::Line,
                    .lineMode = LineMode::Smooth,
                },
                .name = "Raster Pipeline",
            },
            {
                .vertexShaderInfo = { TaskShaderInfo{ .program = vsh } },
                .fragmentShaderInfo = { TaskShaderInfo{ .program = fsh } },
            });
    }
    void WireframeSmooth::ReleaseResources(const ReleaseResourceInfo& info) {
        image = {};
        target = {};
        vsh, fsh = {};
        pipeline = {};
    }
    eastl::span<GenericTask*> WireframeSmooth::CreateTasks() {
        tasks = {
            new GraphicsCallbackTask(
                { .name = "Wireframe Smooth", .color = LabelColor::GREEN },
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