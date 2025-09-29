#include "VertexBuffer.hpp"

namespace VisualTests {
#pragma pack(push, 1)
    struct Vertex {
        eastl::array<f32, 2> position;
        eastl::array<f32, 3> color;
    };
#pragma pack(pop)

    // clang-format off
    static const eastl::vector<Vertex> gVertices = {
        { { 0.0f, 0.5f }, { 1.0f, 0.0f, 0.0f } },
        { {-0.5f,-0.5f }, { 0.0f, 1.0f, 0.0f } },
        { { 0.5f,-0.5f }, { 0.0f, 0.0f, 1.0f } },
        { { 0.4f, 0.8f }, { 1.0f, 1.0f, 0.0f } },
        { {-0.1f,-0.7f }, { 0.0f, 1.0f, 1.0f } },
        { { 0.4f,-0.6f }, { 1.0f, 0.0f, 1.0f } },
    };
    // clang-format on

    void VertexBuffer::CreateResources(const CreateResourceInfo& info) {
        image = info.resourceManager.CreatePersistentImage({
            .format = Format::RGBA8Unorm,
            .size = { info.displayInfo.width, info.displayInfo.height },
            .usage = ImageUsageFlagBits::RENDER_TARGET | ImageUsageFlagBits::TRANSFER_SRC | ImageUsageFlagBits::BLIT_SRC,
            .name = "Vertex Buffer Render Image",
        });
        vbo = info.resourceManager.CreatePersistentBuffer(
            {
                .size = gVertices.size() * sizeof(Vertex),
                .usage = BufferUsageFlagBits::VERTEX_BUFFER,
                .name = "Vertex Buffer VBO",
            },
            { (u8*)gVertices.cbegin(), (u8*)gVertices.cend() });
        target = info.resourceManager.CreateColorTarget({
            .image = image,
            .name = "Vertex Buffer RT",
        });
        vsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/VertexBuffer.slang",
            { .stage = ShaderStage::Vertex, .entryPoint = "vertexMain", .name = "Vertex Buffer Vsh" });
        fsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/VertexBuffer.slang",
            { .stage = ShaderStage::Fragment, .entryPoint = "fragmentMain", .name = "Vertex Buffer Fsh" });
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
    void VertexBuffer::ReleaseResources(const ReleaseResourceInfo& info) {
        image = {};
        vbo = {};
        target = {};
        vsh, fsh = {};
        pipeline = {};
    }
    eastl::span<GenericTask*> VertexBuffer::CreateTasks() {
        tasks = {
            new GraphicsCallbackTask(
                { .name = "Vertex Buffer", .color = LabelColor::GREEN },
                [this](GraphicsTask& task) {
                    task.BindColorTarget({
                        .target = target,
                        .clear = { { 0.0f, 0.0f, 0.0f, 1.0f } },
                    });
                },
                [this](TaskCommandList& commands) {
                    commands.SetRasterPipeline(pipeline);
                    commands.SetVertexBuffer({ .buffer = vbo });
                    commands.Draw({ .vertexCount = 3U });
                    // now try offsetting the vertex buffer
                    commands.SetVertexBuffer({ .buffer = vbo, .offset = 3 * sizeof(Vertex) });
                    commands.Draw({ .vertexCount = 3U });
                })
        };
        return tasks;
    }
} // namespace VisualTests