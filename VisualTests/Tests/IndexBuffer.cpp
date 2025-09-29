#include "IndexBuffer.hpp"

namespace VisualTests {
#pragma pack(push, 1)
    struct Vertex {
        eastl::array<f32, 2> position;
        eastl::array<f32, 3> color;
    };
#pragma pack(pop)

    // Quad vertices (positions + colors)
    static const eastl::vector<Vertex> gVertices = {
        //   Position       //    Color
        { { -0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f } },  // Top-left
        { { 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f } },   // Top-right
        { { 0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f } },  // Bottom-right
        { { -0.5f, -0.5f }, { 1.0f, 1.0f, 0.0f } }, // Bottom-left
    };

    // Two triangles making up the quad (6 indices)
    static const eastl::vector<u32> gIndices = {
        0, 1, 2, // First triangle (Top-left, Top-right, Bottom-right)
        2, 3, 0  // Second triangle (Bottom-right, Bottom-left, Top-left)
    };

    void IndexBuffer::CreateResources(const CreateResourceInfo& info) {
        image = info.resourceManager.CreatePersistentImage({
            .format = Format::RGBA8Unorm,
            .size = { info.displayInfo.width, info.displayInfo.height },
            .usage = ImageUsageFlagBits::RENDER_TARGET | ImageUsageFlagBits::TRANSFER_SRC | ImageUsageFlagBits::BLIT_SRC,
            .name = "Index Buffer Render Image",
        });

        vbo = info.resourceManager.CreatePersistentBuffer(
            {
                .size = gVertices.size() * sizeof(Vertex),
                .usage = BufferUsageFlagBits::VERTEX_BUFFER,
                .name = "Index Buffer VBO",
            },
            { reinterpret_cast<const u8*>(gVertices.data()), reinterpret_cast<const u8*>(gVertices.data() + gVertices.size()) });

        indexBuffer = info.resourceManager.CreatePersistentBuffer(
            {
                .size = gIndices.size() * sizeof(u32),
                .usage = BufferUsageFlagBits::INDEX_BUFFER,
                .name = "Index Buffer Index Buffer",
            },
            { reinterpret_cast<const u8*>(gIndices.data()), reinterpret_cast<const u8*>(gIndices.data() + gIndices.size()) });

        target = info.resourceManager.CreateColorTarget({
            .image = image,
            .name = "Index Buffer RT",
        });

        vsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/IndexBuffer.slang",
            { .stage = ShaderStage::Vertex, .entryPoint = "vertexMain", .name = "Index Buffer Vsh" });
        fsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/IndexBuffer.slang",
            { .stage = ShaderStage::Fragment, .entryPoint = "fragmentMain", .name = "Index Buffer Fsh" });

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

    void IndexBuffer::ReleaseResources(const ReleaseResourceInfo& info) {
        image = {};
        vbo = {};
        target = {};
        vsh = {};
        fsh = {};
        pipeline = {};
        indexBuffer = {};
    }

    eastl::span<GenericTask*> IndexBuffer::CreateTasks() {
        tasks = {
            new GraphicsCallbackTask(
                { .name = "Index Buffer", .color = LabelColor::GREEN },
                [this](GraphicsTask& task) {
                    task.BindColorTarget({
                        .target = target,
                        .clear = { { 0.0f, 0.0f, 0.0f, 1.0f } },
                    });
                },
                [this](TaskCommandList& commands) {
                    commands.SetRasterPipeline(pipeline);
                    commands.SetVertexBuffer({ .buffer = vbo });
                    commands.SetIndexBuffer({ .buffer = indexBuffer, .indexType = IndexType::Uint32 });
                    commands.DrawIndexed({ .indexCount = static_cast<u32>(gIndices.size()) });
                })
        };
        return tasks;
    }
} // namespace VisualTests
