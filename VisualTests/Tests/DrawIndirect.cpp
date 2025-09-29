#include "DrawIndirect.hpp"

namespace VisualTests {
#pragma pack(push, 1)
    struct Vertex {
        eastl::array<f32, 2> position;
        eastl::array<f32, 3> color;
    };

#pragma pack(pop)

    // Triangle vertices
    static const eastl::vector<Vertex> gVertices = {
        { { 0.0f, 0.5f }, { 1.0f, 0.0f, 0.0f } },
        { { -0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f } },
        { { 0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f } },
        { { 0.0f, 0.5f }, { 1.0f, 1.0f, 1.0f } },
    };

    // 3 indirect draws
    static const eastl::vector<DrawArgumentBuffer> gDrawCmds = {
        {
            // normal
            .vertexCount = static_cast<u32>(gVertices.size()),
            .instanceCount = 1,
            .firstVertex = 0,
            .firstInstance = 0,
        },
        {
            // test DrawID
            .vertexCount = static_cast<u32>(gVertices.size()),
            .instanceCount = 1,
            .firstVertex = 0,
            .firstInstance = 1, // this should invert the colours
        },
        {
            // try out offsetting the vertex, should be white
            .vertexCount = static_cast<u32>(gVertices.size()),
            .instanceCount = 1,
            .firstVertex = 1,
            .firstInstance = 0,
        },
    };

    void DrawIndirect::CreateResources(const CreateResourceInfo& info) {
        image = info.resourceManager.CreatePersistentImage({
            .format = Format::RGBA8Unorm,
            .size = { info.displayInfo.width, info.displayInfo.height },
            .usage = ImageUsageFlagBits::RENDER_TARGET | ImageUsageFlagBits::TRANSFER_SRC | ImageUsageFlagBits::BLIT_SRC,
            .name = "Draw Indirect Render Image",
        });

        vbo = info.resourceManager.CreatePersistentBuffer(
            {
                .size = gVertices.size() * sizeof(Vertex),
                .usage = BufferUsageFlagBits::VERTEX_BUFFER,
                .name = "Draw Indirect VBO",
            },
            { reinterpret_cast<const u8*>(gVertices.data()), reinterpret_cast<const u8*>(gVertices.data() + gVertices.size()) });

        indirectBuffer = info.resourceManager.CreatePersistentBuffer(
            {
                .size = gDrawCmds.size() * sizeof(DrawArgumentBuffer),
                .usage = BufferUsageFlagBits::DRAW_INDIRECT,
                .name = "Draw Indirect Argument Buffers",
            },
            { reinterpret_cast<const u8*>(gDrawCmds.cbegin()), reinterpret_cast<const u8*>(gDrawCmds.cend()) });

        target = info.resourceManager.CreateColorTarget({
            .image = image,
            .name = "DrawIndirect RT",
        });

        vsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/DrawIndirect.slang",
            { .stage = ShaderStage::Vertex, .entryPoint = "vertexMain", .name = "DrawIndirect Vsh" });
        fsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/DrawIndirect.slang",
            { .stage = ShaderStage::Fragment, .entryPoint = "fragmentMain", .name = "DrawIndirect Fsh" });

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
                        } },
                    .vertexBindings = eastl::array{
                        VertexBindingInfo{ .binding = 0, .stride = sizeof(Vertex) },
                    },
                },
                .name = "Raster Pipeline",
            },
            {
                .vertexShaderInfo = { TaskShaderInfo{ .program = vsh } },
                .fragmentShaderInfo = { TaskShaderInfo{ .program = fsh } },
            });
    }

    void DrawIndirect::ReleaseResources(const ReleaseResourceInfo& info) {
        image = {};
        vbo = {};
        indirectBuffer = {};
        target = {};
        vsh = {};
        fsh = {};
        pipeline = {};
    }

    eastl::span<GenericTask*> DrawIndirect::CreateTasks() {
        tasks = {
            new GraphicsCallbackTask(
                { .name = "Draw Indirect", .color = LabelColor::GREEN },
                [this](GraphicsTask& task) {
                    task.BindColorTarget({
                        .target = target,
                        .clear = { { 0.0f, 0.0f, 0.0f, 1.0f } },
                    });
                },
                [this](TaskCommandList& commands) {
                    commands.SetRasterPipeline(pipeline);
                    commands.SetVertexBuffer({ .slot = 0, .buffer = vbo });
                    commands.DrawIndirect({
                        .indirectBuffer = indirectBuffer,
                        .drawCount = static_cast<u32>(gDrawCmds.size()),
                    });
                })
        };
        return tasks;
    }
} // namespace VisualTests
