#include "InstanceBuffer.hpp"

namespace VisualTests {
#pragma pack(push, 1)
    struct Vertex {
        eastl::array<f32, 2> position;
        eastl::array<f32, 3> color;
    };

    struct InstanceData {
        eastl::array<f32, 2> offset;
    };
#pragma pack(pop)

    // Triangle vertices
    static const eastl::vector<Vertex> gVertices = {
        { { 0.0f, 0.5f }, { 1.0f, 0.0f, 0.0f } },
        { { -0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f } },
        { { 0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f } },
    };

    // Per-instance offsets
    static const eastl::vector<InstanceData> gInstances = {
        { { -0.6f, 0.0f } },
        { { 0.6f, 0.0f } },
        { { 0.0f, 0.6f } },
    };

    void InstanceBuffer::CreateResources(const CreateResourceInfo& info) {
        image = info.resourceManager.CreatePersistentImage({
            .format = Format::RGBA8Unorm,
            .size = { info.displayInfo.width, info.displayInfo.height },
            .usage = ImageUsageFlagBits::RENDER_TARGET | ImageUsageFlagBits::TRANSFER_SRC | ImageUsageFlagBits::BLIT_SRC,
            .name = "Instance Buffer Render Image",
        });

        vbo = info.resourceManager.CreatePersistentBuffer(
            {
                .size = gVertices.size() * sizeof(Vertex),
                .usage = BufferUsageFlagBits::VERTEX_BUFFER,
                .name = "Instance Buffer VBO",
            },
            { reinterpret_cast<const u8*>(gVertices.data()), reinterpret_cast<const u8*>(gVertices.data() + gVertices.size()) });

        ibo = info.resourceManager.CreatePersistentBuffer(
            {
                .size = gInstances.size() * sizeof(InstanceData),
                .usage = BufferUsageFlagBits::VERTEX_BUFFER,
                .name = "Instance Buffer IBO",
            },
            { reinterpret_cast<const u8*>(gInstances.data()), reinterpret_cast<const u8*>(gInstances.data() + gInstances.size()) });

        target = info.resourceManager.CreateColorTarget({
            .image = image,
            .name = "Instance Buffer RT",
        });

        vsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/InstanceBuffer.slang",
            { .stage = ShaderStage::Vertex, .entryPoint = "vertexMain", .name = "Instance Buffer Vsh" });
        fsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/InstanceBuffer.slang",
            { .stage = ShaderStage::Fragment, .entryPoint = "fragmentMain", .name = "Instance Buffer Fsh" });

        pipeline = info.resourceManager.CreateRasterPipeline(
            {
                .colorTargetStates = { { .format = image->Info().format } },
                .inputAssemblyState = {
                    .primitiveTopology = PrimitiveTopology::TriangleList,
                    .vertexAttributes = eastl::array{
                        VertexAttributeInfo{
                            .location = 0,
                            .binding = 0,
                            .format = Format::RG32Sfloat,
                            .offset = offsetof(Vertex, position),
                        },
                        VertexAttributeInfo{
                            .location = 1,
                            .binding = 0,
                            .format = Format::RGB32Sfloat,
                            .offset = offsetof(Vertex, color),
                        },
                        VertexAttributeInfo{
                            .location = 2,
                            .binding = 1,
                            .format = Format::RG32Sfloat,
                            .offset = offsetof(InstanceData, offset),
                        } },
                    .vertexBindings = eastl::array{
                        VertexBindingInfo{ .binding = 0, .stride = sizeof(Vertex), .bPerInstance = false },
                        VertexBindingInfo{ .binding = 1, .stride = sizeof(InstanceData), .bPerInstance = true },
                    },
                },
                .name = "Raster Pipeline",
            },
            {
                .vertexShaderInfo = { TaskShaderInfo{ .program = vsh } },
                .fragmentShaderInfo = { TaskShaderInfo{ .program = fsh } },
            });
    }

    void InstanceBuffer::ReleaseResources(const ReleaseResourceInfo& info) {
        image = {};
        vbo = {};
        ibo = {};
        target = {};
        vsh = {};
        fsh = {};
        pipeline = {};
    }

    eastl::span<GenericTask*> InstanceBuffer::CreateTasks() {
        tasks = {
            new GraphicsCallbackTask(
                { .name = "Instance Buffer", .color = LabelColor::GREEN },
                [this](GraphicsTask& task) {
                    task.BindColorTarget({
                        .target = target,
                        .clear = { { 0.0f, 0.0f, 0.0f, 1.0f } },
                    });
                },
                [this](TaskCommandList& commands) {
                    commands.SetRasterPipeline(pipeline);
                    commands.SetVertexBuffer({ .slot = 0, .buffer = vbo });
                    commands.SetVertexBuffer({ .slot = 1, .buffer = ibo });
                    commands.Draw({ .vertexCount = static_cast<u32>(gVertices.size()), .instanceCount = static_cast<u32>(gInstances.size()) });
                })
        };
        return tasks;
    }
} // namespace VisualTests
