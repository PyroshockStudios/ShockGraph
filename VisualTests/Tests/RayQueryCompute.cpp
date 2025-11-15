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

#include "RayQueryCompute.hpp"
#include <PyroRHI/Context.hpp>

namespace VisualTests {
    // --- Build geometry buffers for BLAS/TLAS
    struct SimpleVertex {
        f32 x, y, z;
    };
    void RayQueryCompute::CreateResources(const CreateResourceInfo& info) {
        IDevice* device = info.resourceManager.GetInternalDevice();

        device->SetShaderModel(info.resourceManager.GetInternalContext()->GetMinimumShaderModelFeatureTier(ShaderModelFeatureBits::RAY_QUERY));

        image = info.resourceManager.CreatePersistentImage({
            .format = Format::RGBA32Sfloat,
            .size = { info.displayInfo.width, info.displayInfo.height },
            .usage = ImageUsageFlagBits::UNORDERED_ACCESS | ImageUsageFlagBits::TRANSFER_SRC | ImageUsageFlagBits::BLIT_SRC,
            .name = "Ray-Query Compute Image",
        });

        imageUav = info.resourceManager.CreateUnorderedAccessView({ .image = image });

        csh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/RayQueryCompute.slang",
            { .stage = ShaderStage::Compute, .entryPoint = "computeMain", .name = "RayQuery Compute" });

        computePipeline = info.resourceManager.CreateComputePipeline({ .name = "RayQuery Compute Pipeline" }, { .program = csh });


        const SimpleVertex vertices[] = {
            { -1.f, -1.f, 4.f },
            { 1.f, -1.f, 4.f },
            { 1.f, 1.f, 4.f },
            { -1.f, 1.f, 4.f }
        };
        const u32 indices[] = { 0, 1, 2, 0, 2, 3 };

        // Create vertex/index buffers with initial data
        vertexBuffer = info.resourceManager.CreatePersistentBuffer(
            {
                .size = sizeof(vertices),
                .usage = BufferUsageFlagBits::BLAS_GEOMETRY_BUFFER,
                .mode = TaskBufferMode::Default,
                .name = "RT Vertices",
            },
            eastl::span<const u8>(reinterpret_cast<const u8*>(vertices), sizeof(vertices)));

        indexBuffer = info.resourceManager.CreatePersistentBuffer(
            {
                .size = sizeof(indices),
                .usage = BufferUsageFlagBits::BLAS_GEOMETRY_BUFFER,
                .mode = TaskBufferMode::Default,
                .name = "RT Indices",
            },
            eastl::span<const u8>(reinterpret_cast<const u8*>(indices), sizeof(indices)));

        // Prepare RHI Blas geometry info
        BlasTriangleGeometryInfo triGeo{};
        triGeo.flags = AccelerationStructureGeometryFlagBits::OPAQUE | AccelerationStructureGeometryFlagBits::NO_DUPLICATE_ANY_HIT_INVOCATION;
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

        // Instance buffer for TLAS
        BlasInstanceData instanceData{};
        instanceData.transform = Transform::IDENTITY;
        instanceData.instanceCustomIndex = 0;
        instanceData.mask = 0xFF;
        instanceData.instanceShaderBindingTableRecordOffset = 0;
        instanceData.flags = AccelerationStructureGeometryInstanceFlagBits::FORCE_OPAQUE |
                             AccelerationStructureGeometryInstanceFlagBits::TRIANGLE_FACING_CULL_DISABLE;
        instanceData.blasAddress = device->BlasInstanceAddress(blas->Internal());

        instanceBuffer = info.resourceManager.CreatePersistentBuffer(
            {
                .size = sizeof(BlasInstanceData),
                .usage = BufferUsageFlagBits::BLAS_INSTANCE_BUFFER,
                .mode = TaskBufferMode::Default,
                .name = "RT Instance Buffer",
            },
            eastl::span<const u8>(reinterpret_cast<const u8*>(&instanceData), sizeof(instanceData)));

        // TLAS size requirements
        TlasInstanceInfo tlasInstanceInfo = { .data = instanceBuffer->Internal(), .count = 1 };
        TlasBuildInfo tlasBuildInfo = { .instances = tlasInstanceInfo };
        AccelerationStructureBuildSizesInfo tlasSizeInfo = device->TlasSizeRequirements(tlasBuildInfo);

        // Create TLAS and scratch
        tlas = info.resourceManager.CreatePersistentTlas({ .size = tlasSizeInfo.accelerationStructureSize, .name = "RT Tlas" });
        tlasScratchBuffer = info.resourceManager.CreatePersistentBuffer({
            .size = tlasSizeInfo.buildScratchSize,
            .usage = BufferUsageFlagBits::ACCELERATION_STRUCTURE_SCRATCH_BUFFER,
            .name = "RT Tlas Scratch",
        });
    }


    void RayQueryCompute::ReleaseResources(const ReleaseResourceInfo& info) {
        info.resourceManager.ReleaseUnorderedAccessView(imageUav);
        computePipeline = {};
        image = {};
        csh = {};
        vertexBuffer = {};
        indexBuffer = {};
        instanceBuffer = {};
        blasScratchBuffer = {};
        tlasScratchBuffer = {};
        blas = {};
        tlas = {};
        bBuilt = false;
    }

    eastl::span<GenericTask*> RayQueryCompute::CreateTasks() {
        // Task 1: Build BLAS
        GenericTask* buildBlasTask = new CustomCallbackTask(
            { .name = "Build Bottom Level Acceleration Structures", .color = LabelColor::YELLOW },
            [this](CustomTask& task) {
                // declare buffer usage so task graph orders correctly
                task.UseAccelerationStructure({ .accelerationStructure = blas, .access = AccessConsts::ACCELERATION_STRUCTURE_BUILD_READ_WRITE });
            },
            [this](ICommandBuffer* commands) {
                //  Prepare RHI build infos
                BlasTriangleGeometryInfo triGeo{};
                triGeo.flags = AccelerationStructureGeometryFlagBits::OPAQUE | AccelerationStructureGeometryFlagBits::NO_DUPLICATE_ANY_HIT_INVOCATION;
                triGeo.vertexFormat = Format::RGB32Sfloat;
                triGeo.indexType = IndexType::Uint32;
                triGeo.vertexBuffer = vertexBuffer->Internal();
                triGeo.indexBuffer = indexBuffer->Internal();
                triGeo.vertexStride = sizeof(SimpleVertex);
                triGeo.vertexCount = 4;
                triGeo.indexCount = 6;

                eastl::array<BlasTriangleGeometryInfo, 1> geoArray = { triGeo };
                BlasBuildInfo blasBuildInfo{};
                blasBuildInfo.geometries = geoArray;
                blasBuildInfo.dstBlas = blas->Internal();
                blasBuildInfo.scratchBuffer = blasScratchBuffer->Internal();

                BuildAccelerationStructuresInfo buildInfo = {
                    .blasBuildInfos = eastl::span<const BlasBuildInfo>(&blasBuildInfo, 1),
                };

                commands->BuildAccelerationStructures(buildInfo);
            },
            TaskType::Transfer);
        // Task 2: build Tlas
        GenericTask* buildTlasTask = new CustomCallbackTask(
            { .name = "Build Top Level Acceleration Structures", .color = LabelColor::YELLOW },
            [this](CustomTask& task) {
                // declare buffer usage so task graph orders correctly
                task.UseAccelerationStructure({ .accelerationStructure = blas, .access = AccessConsts::ACCELERATION_STRUCTURE_BUILD_READ });
                task.UseAccelerationStructure({ .accelerationStructure = tlas, .access = AccessConsts::ACCELERATION_STRUCTURE_BUILD_READ_WRITE });
            },
            [this](ICommandBuffer* commands) {
                TlasInstanceInfo tlasInstanceInfo = { .data = instanceBuffer->Internal(), .count = 1 };
                tlasInstanceInfo.flags = AccelerationStructureGeometryFlagBits::OPAQUE | AccelerationStructureGeometryFlagBits::NO_DUPLICATE_ANY_HIT_INVOCATION;

                TlasBuildInfo tlasBuildInfo{};
                tlasBuildInfo.instances = tlasInstanceInfo;
                tlasBuildInfo.dstTlas = tlas->Internal();
                tlasBuildInfo.scratchBuffer = tlasScratchBuffer->Internal();

                BuildAccelerationStructuresInfo buildInfo = {
                    .tlasBuildInfos = eastl::span<const TlasBuildInfo>(&tlasBuildInfo, 1),
                };

                commands->BuildAccelerationStructures(buildInfo);
            },
            TaskType::Transfer);

        // Task 2: Dispatch compute shader which performs ray queries
        GenericTask* dispatchTask = new ComputeCallbackTask(
            { .name = "RayQuery Compute Dispatch", .color = LabelColor::YELLOW },
            [this](ComputeTask& task) {
                task.UseImage({ .image = image, .access = AccessConsts::COMPUTE_SHADER_WRITE });
                task.UseAccelerationStructure({ .accelerationStructure = tlas, .access = AccessConsts::COMPUTE_SHADER_READ });
            },
            [this](TaskCommandList& commands) {
                commands.SetComputePipeline(computePipeline);
                // push TLAS index so shader can index the descriptor array
                u32 tlasIndex = tlas->Internal().index;
                commands.PushConstant(tlasIndex);
                commands.SetUnorderedAccessView({ .slot = 0, .view = imageUav });
                // dispatch in 8x8 workgroups
                Extent3D dim = image->Info().size;
                u32 gx = (dim.width + 7) / 8;
                u32 gy = (dim.height + 7) / 8;
                commands.Dispatch({ .x = gx, .y = gy });
            });

        tasks = { buildBlasTask, buildTlasTask, dispatchTask };

        return tasks;
    }
    bool RayQueryCompute::TaskSupported(IDevice* device) { return device->Features().bAccelerationStructureBuild&&
                                                                  device->Features().bRayQueries; }

} // namespace VisualTests
