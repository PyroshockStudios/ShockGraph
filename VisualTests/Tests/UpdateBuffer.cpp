#include "UpdateBuffer.hpp"

namespace VisualTests {

    void UpdateBuffer::CreateResources(const CreateResourceInfo& info) {
        image = info.resourceManager.CreatePersistentImage({
            .format = Format::RGBA8Unorm,
            .size = { info.displayInfo.width, info.displayInfo.height },
            .usage = ImageUsageFlagBits::RENDER_TARGET | ImageUsageFlagBits::TRANSFER_SRC | ImageUsageFlagBits::BLIT_SRC,
            .name = "Update Buffer Render Image",
        });
        ubo = info.resourceManager.CreatePersistentBuffer({
            .size = sizeof(f32),
            .usage = BufferUsageFlagBits::UNIFORM_BUFFER | BufferUsageFlagBits::TRANSFER_DST /*Transfer DST is required*/,
            .bCpuVisible = false, // Not cpu visible! Should not be  needed for UpdateBuffer()!
            .bDynamic = false,    // Not dynamic! Should not be needed either!
            .name = "Vertex Scale Uniform Buffer",
        });
        target = info.resourceManager.CreateColorTarget({
            .image = image,
            .name = "Update Buffer RT",
        });
        vsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/UpdateBuffer.slang",
            { .stage = ShaderStage::Vertex, .entryPoint = "vertexMain", .name = "Update Buffer Vsh" });
        fsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/UpdateBuffer.slang",
            { .stage = ShaderStage::Fragment, .entryPoint = "fragmentMain", .name = "Update Buffer Fsh" });
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
    void UpdateBuffer::ReleaseResources(const ReleaseResourceInfo& info) {
        image = {};
        ubo = {};
        target = {};
        vsh, fsh = {};
        pipeline = {};
    }
    eastl::span<GenericTask*> UpdateBuffer::CreateTasks() {
        tasks = {
            // Update buffer is basically a transfer operation,
            // Access the buffer with TRANSFER_WRITE
            new TransferCallbackTask(
                { .name = "Update Buffer", .color = LabelColor::GREEN },
                [this](TransferTask& task) {
                    task.UseBuffer({ .buffer = ubo,
                        .access = AccessConsts::TRANSFER_WRITE });
                },
                [this](TaskCommandList& commands) {
                    static float time = 0.0f;
                    time += 1.0f / 200.0f;
                    float scale = sin(time) * 0.5f + 0.5f;
                    commands.UpdateBuffer({
                        .buffer = ubo,
                        .data = &scale,
                    });
                }),
            new GraphicsCallbackTask(
                { .name = "Read Buffer", .color = LabelColor::GREEN },
                [this](GraphicsTask& task) {
                    task.BindColorTarget({
                        .target = target,
                        .clear = { { 0.0f, 0.0f, 0.0f, 1.0f } },
                    });
                    // Finally, read from the buffer
                    task.UseBuffer({ .buffer = ubo,
                        .access = AccessConsts::VERTEX_SHADER_READ });
                },
                [this](TaskCommandList& commands) {
                    commands.SetRasterPipeline(pipeline);
                    commands.SetUniformBufferView({
                        .slot = 0,
                        .buffer = ubo,
                    });

                    commands.Draw({ .vertexCount = 3 });
                })
        };
        return tasks;
    }
} // namespace VisualTests