// MIT License
//
// Copyright (c) 2025 Pyroshock Studios
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "ComputeUAV.hpp"

namespace VisualTests {
#pragma pack(push, 1)
    struct Vertex {
        eastl::array<f32, 4> position;
        eastl::array<f32, 4> color;
    };
#pragma pack(pop)
    static const u32 GRID_SIZE_U = 64; // around the main ring
    static const u32 GRID_SIZE_V = 32; // around the tube

    static const u32 VERTEX_COUNT = GRID_SIZE_U * GRID_SIZE_V;
    static const u32 INDEX_COUNT = (GRID_SIZE_U - 1) * (GRID_SIZE_V - 1) * 6;

    void ComputeUAV::CreateResources(const CreateResourceInfo& info) {
        image = info.resourceManager.CreatePersistentImage({
            .format = Format::RGBA8Unorm,
            .size = { info.displayInfo.width, info.displayInfo.height },
            .usage = ImageUsageFlagBits::RENDER_TARGET | ImageUsageFlagBits::TRANSFER_SRC | ImageUsageFlagBits::BLIT_SRC,
            .name = "Compute-UAV Render Image",
        });
        depth = info.resourceManager.CreatePersistentImage({
            .format = Format::D32Sfloat,
            .size = { info.displayInfo.width, info.displayInfo.height },
            .usage = ImageUsageFlagBits::RENDER_TARGET,
            .name = "Compute-UAV Depth",
        });
        vboUav = info.resourceManager.CreatePersistentBuffer(
            {
                .size = VERTEX_COUNT * sizeof(Vertex),
                .usage = BufferUsageFlagBits::VERTEX_BUFFER | BufferUsageFlagBits::UNORDERED_ACCESS,
                .name = "Compute-UAV VBO/UAV",
            });
        idxUav = info.resourceManager.CreatePersistentBuffer(
            {
                .size = INDEX_COUNT * sizeof(u32),
                .usage = BufferUsageFlagBits::INDEX_BUFFER | BufferUsageFlagBits::UNORDERED_ACCESS,
                .name = "Compute-UAV Index buffer/UAV",
            });
        vboUaView = info.resourceManager.CreateUnorderedAccessView({ .buffer = vboUav });
        idxUaView = info.resourceManager.CreateUnorderedAccessView({ .buffer = idxUav });
        target = info.resourceManager.CreateColorTarget({
            .image = image,
            .name = "Compute-UAV RT",
        });
        depthTarget = info.resourceManager.CreateDepthStencilTarget({
            .image = depth,
            .bDepth = true,
            .name = "Compute-UAV DS",
        });
        vsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/ComputeUAV.slang",
            { .stage = ShaderStage::Vertex, .entryPoint = "vertexMain", .name = "Compute-UAV Vsh" });
        fsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/ComputeUAV.slang",
            { .stage = ShaderStage::Fragment, .entryPoint = "fragmentMain", .name = "Compute-UAV Fsh" });
        csh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/ComputeUAV.slang",
            { .stage = ShaderStage::Compute, .entryPoint = "computeMain", .name = "Compute-UAV Csh" });
        renderVertices = info.resourceManager.CreateRasterPipeline(
            {
                .colorTargetStates = { { .format = image->Info().format } },
                .depthStencilState = eastl::make_optional<DepthStencilStateInfo>(DepthStencilStateInfo{
                    .depthStencilFormat = depth->Info().format,
                    .depthTestState = DepthStencilTestState::ReadWrite,
                    .depthTest = CompareOp::Greater,
                }),
                .inputAssemblyState = {
                    .primitiveTopology = PrimitiveTopology::TriangleList,
                    .vertexAttributes = eastl::array{
                        VertexAttributeInfo{
                            .location = 0,
                            .binding = 0,
                            .format = Format::RGBA32Sfloat,
                            .offset = offsetof(Vertex, position),
                        },
                        VertexAttributeInfo{
                            .location = 1,
                            .binding = 0,
                            .format = Format::RGBA32Sfloat,
                            .offset = offsetof(Vertex, color),
                        } },
                    .vertexBindings = eastl::array{
                        VertexBindingInfo{ .binding = 0, .stride = sizeof(Vertex) },
                    },
                },
                .name = "Render Vertices Pipeline",
            },
            {
                .vertexShaderInfo = { TaskShaderInfo{ .program = vsh } },
                .fragmentShaderInfo = { TaskShaderInfo{ .program = fsh } },
            });

        generateVertices = info.resourceManager.CreateComputePipeline(
            { .name = "Generate Vertices Pipeline" },
            { .program = csh });
    }
    void ComputeUAV::ReleaseResources(const ReleaseResourceInfo& info) {
        info.resourceManager.ReleaseUnorderedAccessView(vboUaView);
        info.resourceManager.ReleaseUnorderedAccessView(idxUaView);
        image = {};
        depth = {};
        vboUav = {};
        idxUav = {};
        target = {};
        depthTarget = {};
        vsh = {}; fsh = {};
        renderVertices = {};
        generateVertices = {};
    }
    eastl::span<GenericTask*> ComputeUAV::CreateTasks() {
        tasks = {
            new ComputeCallbackTask(
                { .name = "Compute-UAV Generate Vertices", .color = LabelColor::YELLOW },
                [this](ComputeTask& task) {
                    task.UseBuffer({
                        .buffer = vboUav,
                        .access = AccessConsts::COMPUTE_SHADER_WRITE,
                    });
                    task.UseBuffer({
                        .buffer = idxUav,
                        .access = AccessConsts::COMPUTE_SHADER_WRITE,
                    });
                },
                [this](TaskCommandList& commands) {
                    static f32 time = 0.0f;
                    commands.SetComputePipeline(generateVertices);
                    commands.SetUnorderedAccessView({ .slot = 0, .view = vboUaView });
                    commands.SetUnorderedAccessView({ .slot = 1, .view = idxUaView });
                    commands.PushConstant(time);
                    commands.Dispatch({ .x = (GRID_SIZE_U + 7) / 8, .y = (GRID_SIZE_V + 7) / 8 });

                    time += 1.0f / 200.0f;
                }),
            new GraphicsCallbackTask(
                { .name = "Compute-UAV Draw Vertices", .color = LabelColor::GREEN },
                [this](GraphicsTask& task) {
                    task.BindColorTarget({
                        .target = target,
                        .clear = { { 0.0f, 0.0f, 0.0f, 1.0f } },
                    });
                    task.BindDepthStencilTarget({
                        .target = depthTarget,
                        .depthClear = 0.0f,
                    });
                    task.UseBuffer({
                        .buffer = vboUav,
                        .access = AccessConsts::VERTEX_INPUT_READ,
                    });
                    task.UseBuffer({
                        .buffer = idxUav,
                        .access = AccessConsts::INDEX_INPUT_READ,
                    });
                },
                [this](TaskCommandList& commands) {
                    static f32 time = 0.0f;
                    time += 1.0f / 200.0f;
                    commands.SetRasterPipeline(renderVertices);
                    commands.SetVertexBuffer({ .buffer = vboUav });
                    commands.SetIndexBuffer({ .buffer = idxUav, .indexType = IndexType::Uint32 });
                    commands.PushConstant(time);
                    commands.DrawIndexed({ .indexCount = INDEX_COUNT });
                })
        };
        return tasks;
    }
} // namespace VisualTests
