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

#include "RayQueryPixel.hpp"

namespace VisualTests {
    // --- Build geometry buffers for BLAS/TLAS
    struct SimpleVertex {
        f32 x, y, z;
    };
    void RayQueryPixel::CreateResources(const CreateResourceInfo& info) {
        image = info.resourceManager.CreatePersistentImage({
            .format = Format::RGBA8Unorm,
            .size = { info.displayInfo.width, info.displayInfo.height },
            .usage = ImageUsageFlagBits::RENDER_TARGET | ImageUsageFlagBits::TRANSFER_SRC | ImageUsageFlagBits::BLIT_SRC,
            .name = "Hello Texture Render Image",
        });

        imageTarget = info.resourceManager.CreateColorTarget({
            .image = image,
            .name = "Hello Texture RT",
        });

        vsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/RayQuery.slang",
            { .stage = ShaderStage::Vertex, .entryPoint = "vertexMain", .name = "RayQuery Vsh" });
        fsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/RayQuery.slang",
            { .stage = ShaderStage::Fragment, .entryPoint = "fragmentMain", .name = "RayQuery Fsh" });
        pipeline = info.resourceManager.CreateRasterPipeline(
            {
                .colorTargetStates = { { .format = image->Info().format } },
                .name = "Raster Pipeline",
            },
            {
                .vertexShaderInfo = { TaskShaderInfo{ .program = vsh } },
                .fragmentShaderInfo = { TaskShaderInfo{ .program = fsh } },
            });

        // Create vertex/index buffers with initial data
        vertexBuffer = info.resourceManager.CreatePersistentBuffer(
            {
                .size = sizeof(SimpleVertex) * 4,
                .usage = BufferUsageFlagBits::BLAS_GEOMETRY_BUFFER,
                .mode = TaskBufferMode::HostDynamic,
                .name = "RT Vertices",
            });

        indexBuffer = info.resourceManager.CreatePersistentBuffer(
            {
                .size = sizeof(u32) * 6,
                .usage = BufferUsageFlagBits::BLAS_GEOMETRY_BUFFER,
                .mode = TaskBufferMode::HostDynamic,
                .name = "RT Indices",
            });

        // Get the internal device to query size requirements
        IDevice* device = info.resourceManager.GetInternalDevice();

        // Prepare RHI Blas geometry info
        BlasTriangleGeometryInfo triGeo{};
        triGeo.flags = AccelerationStructureGeometryFlagBits::OPAQUE;
        triGeo.vertexFormat = Format::RGB32Sfloat;
        triGeo.indexType = IndexType::Uint32;
        triGeo.vertexBuffer = vertexBuffer->Internal();
        triGeo.indexBuffer = indexBuffer->Internal();
        triGeo.vertexStride = sizeof(SimpleVertex);
        triGeo.vertexCount = 4;
        triGeo.indexCount = 6;

        eastl::array<BlasTriangleGeometryInfo, 1> geoArray = { triGeo };
        BlasBuildInfo blasBuildInfo = { .geometries = geoArray };

        AccelerationStructureBuildSizesInfo blasSizeInfo = device->BlasSizeRequirements(blasBuildInfo);

        // Create BLAS resource
        blas = info.resourceManager.CreatePersistentBlas({ .size = blasSizeInfo.accelerationStructureSize, .name = "RT Blas" });

        // Create scratch buffers for BLAS and TLAS
        blasScratchBuffer = info.resourceManager.CreatePersistentBuffer({
            .size = blasSizeInfo.buildScratchSize,
            .usage = BufferUsageFlagBits::ACCELERATION_STRUCTURE_SCRATCH_BUFFER,
            .name = "RT Blas Scratch",
        });


        instanceBuffer = info.resourceManager.CreatePersistentBuffer(
            {
                .size = sizeof(BlasInstanceData),
                .usage = BufferUsageFlagBits::BLAS_INSTANCE_BUFFER,
                .mode = TaskBufferMode::HostDynamic,
                .name = "RT Instance Buffer",
            });

        // TLAS size requirements
        TlasInstanceInfo tlasInstanceInfo = { .data = instanceBuffer->Internal(), .count = 1 };
        tlasInstanceInfo.flags = AccelerationStructureGeometryFlagBits::OPAQUE;
        TlasBuildInfo tlasBuildInfo = { .instances = tlasInstanceInfo };
        AccelerationStructureBuildSizesInfo tlasSizeInfo = device->TlasSizeRequirements(tlasBuildInfo);

        // Create TLAS and scratch
        tlas = info.resourceManager.CreatePersistentTlas({ .size = tlasSizeInfo.accelerationStructureSize, .name = "RT Tlas" });
        tlasScratchBuffer = info.resourceManager.CreatePersistentBuffer({
            .size = tlasSizeInfo.buildScratchSize,
            .usage = BufferUsageFlagBits::ACCELERATION_STRUCTURE_SCRATCH_BUFFER,
            .name = "RT Tlas Scratch",
        });


        // build AS

        const SimpleVertex vertices[] = {
            { -1.f, -1.f, 4.f },
            { 1.f, -1.f, 4.f },
            { 1.f, 1.f, 4.f },
            { -1.f, 1.f, 4.f }
        };
        memcpy(vertexBuffer->MappedMemory(), vertices, sizeof(vertices));
        const u32 indices[] = { 0, 1, 2, 0, 2, 3 };
        memcpy(indexBuffer->MappedMemory(), indices, sizeof(indices));

        // Instance buffer for TLAS
        BlasInstanceData& instanceData = *reinterpret_cast<BlasInstanceData*>(instanceBuffer->MappedMemory());
        instanceData.transform = Transform::IDENTITY;
        instanceData.instanceCustomIndex = 0;
        instanceData.mask = 0xFF;
        instanceData.instanceShaderBindingTableRecordOffset = 0;
        instanceData.flags = AccelerationStructureGeometryInstanceFlagBits::TRIANGLE_FACING_CULL_DISABLE;
        instanceData.blasAddress = blas->InstanceAddress();


        blasBuildInfo.geometries = geoArray;
        blasBuildInfo.dstBlas = blas->Internal();
        blasBuildInfo.scratchBuffer = blasScratchBuffer->Internal();

        tlasBuildInfo.instances = tlasInstanceInfo;
        tlasBuildInfo.dstTlas = tlas->Internal();
        tlasBuildInfo.scratchBuffer = tlasScratchBuffer->Internal();

        BuildAccelerationStructuresInfo buildAllInfo = {
            .tlasBuildInfos = eastl::span<const TlasBuildInfo>(&tlasBuildInfo, 1),
            .blasBuildInfos = eastl::span<const BlasBuildInfo>(&blasBuildInfo, 1),
        };

        ICommandBuffer* singleTimeCommands = device->GetPresentQueue()->GetCommandBuffer({ .name = "Single time build commands" });
        singleTimeCommands->BuildAccelerationStructures(buildAllInfo);
        singleTimeCommands->Complete();
        device->GetPresentQueue()->SubmitCommandBuffer(singleTimeCommands);
        device->SubmitQueue({ .queue = device->GetPresentQueue() });
        device->WaitIdle();
    }


    void RayQueryPixel::ReleaseResources(const ReleaseResourceInfo& info) {
        pipeline = {};
        imageTarget = {};
        image = {};
        fsh = {};
        vsh = {};
        vertexBuffer = {};
        indexBuffer = {};
        instanceBuffer = {};
        blasScratchBuffer = {};
        tlasScratchBuffer = {};
        blas = {};
        tlas = {};
        bBuilt = false;
    }

    eastl::span<GenericTask*> RayQueryPixel::CreateTasks() {

        // Task 2: Dispatch compute shader which performs ray queries
        GenericTask* drawTask = new GraphicsCallbackTask(
            { .name = "RayQuery Draw Call", .color = LabelColor::YELLOW },
            [this](GraphicsTask& task) {
                task.BindColorTarget({
                    .target = imageTarget,
                    .clear = eastl::make_optional<ColorClearValue>(eastl::array<f32, 4>{ 0, 0, 0, 0 }),
                });
                task.UseAccelerationStructure({ .accelerationStructure = blas, .access = AccessConsts::FRAGMENT_SHADER_READ });
                task.UseAccelerationStructure({ .accelerationStructure = tlas, .access = AccessConsts::FRAGMENT_SHADER_READ });
            },
            [this](TaskCommandList& commands) {
                commands.SetRasterPipeline(pipeline);
                // push TLAS index so shader can index the descriptor array
                u32 tlasIndex = tlas->Internal().index;
                commands.PushConstant(tlasIndex);
                commands.Draw({ .vertexCount = 6 });
            });

        tasks = { drawTask };

        return tasks;
    }

} // namespace VisualTests
