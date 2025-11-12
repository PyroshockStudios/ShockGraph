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

#include "RayTracingCompute.hpp"

namespace VisualTests {

    void RayTracingCompute::CreateResources(const CreateResourceInfo& info) {
        image = info.resourceManager.CreatePersistentImage({
            .format = Format::RGBA8Unorm,
            .size = { info.displayInfo.width, info.displayInfo.height },
            .usage = ImageUsageFlagBits::UNORDERED_ACCESS | ImageUsageFlagBits::TRANSFER_SRC | ImageUsageFlagBits::BLIT_SRC,
            .name = "Ray-Query Compute Image",
        });

        imageUav = info.resourceManager.CreateUnorderedAccessView({ .image = image });

        csh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/RayQuery.slang",
            { .stage = ShaderStage::Compute, .entryPoint = "computeMain", .name = "RayQuery Compute" });

        computePipeline = info.resourceManager.CreateComputePipeline({ .name = "RayQuery Compute Pipeline" }, { .program = csh });

        // --- Build geometry buffers for BLAS/TLAS
        struct SimpleVertex { f32 x, y, z; };

        const SimpleVertex vertices[] = { { 3.f, 3.f, 4.f }, { -3.f, 3.f, 4.f }, { 0.f, -3.f, 4.f }, { 0.f, 0.f, 4.f } };
        const u32 indices[] = { 0, 1, 2, 1, 2, 3 };

        // Create vertex/index buffers with initial data
        vertexBuffer = info.resourceManager.CreatePersistentBuffer({
            .size = sizeof(vertices),
            .usage = BufferUsageFlagBits::BLAS_GEOMETRY_BUFFER,
            .bCpuVisible = true,
            .name = "RT Vertices",
        }, eastl::span<const u8>(reinterpret_cast<const u8*>(vertices), sizeof(vertices)));

        indexBuffer = info.resourceManager.CreatePersistentBuffer({
            .size = sizeof(indices),
            .usage = BufferUsageFlagBits::BLAS_GEOMETRY_BUFFER,
            .bCpuVisible = true,
            .name = "RT Indices",
        }, eastl::span<const u8>(reinterpret_cast<const u8*>(indices), sizeof(indices)));

        // Get the internal device to query size requirements
        IDevice* device = info.resourceManager.GetInternalDevice();

        // Prepare RHI Blas geometry info
        BlasTriangleGeometryInfo triGeo{};
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
        instanceData.flags = 0;
        instanceData.blasAddress = device->BlasInstanceAddress(blas->Internal());

        instanceBuffer = info.resourceManager.CreatePersistentBuffer({
            .size = sizeof(BlasInstanceData),
            .usage = BufferUsageFlagBits::BLAS_INSTANCE_BUFFER,
            .bCpuVisible = true,
            .name = "RT Instance Buffer",
        }, eastl::span<const u8>(reinterpret_cast<const u8*>(&instanceData), sizeof(instanceData)));

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


    void RayTracingCompute::ReleaseResources(const ReleaseResourceInfo& info) {
        info.resourceManager.ReleaseUnorderedAccessView(imageUav);
        image = {};
        csh = {};
        computePipeline = {};
    }

    eastl::span<GenericTask*> RayTracingCompute::CreateTasks() {
        // Task 0: Build BLAS/TLAS
        GenericTask* buildTask = new CustomCallbackTask(
            { .name = "Build Acceleration Structures", .color = LabelColor::YELLOW },
            [this](CustomTask& task) {
                // declare buffer usage so task graph orders correctly
                task.UseBuffer({ .buffer = vertexBuffer, .access = AccessConsts::ACCELERATION_STRUCTURE_BUILD_READ_WRITE });
                task.UseBuffer({ .buffer = indexBuffer, .access = AccessConsts::ACCELERATION_STRUCTURE_BUILD_READ_WRITE });
                task.UseBuffer({ .buffer = instanceBuffer, .access = AccessConsts::ACCELERATION_STRUCTURE_BUILD_READ_WRITE });
                task.UseBuffer({ .buffer = blasScratchBuffer, .access = AccessConsts::ACCELERATION_STRUCTURE_BUILD_READ_WRITE });
                task.UseBuffer({ .buffer = tlasScratchBuffer, .access = AccessConsts::ACCELERATION_STRUCTURE_BUILD_READ_WRITE });
                task.UseTlas({ .tlas = tlas, .access = AccessConsts::ACCELERATION_STRUCTURE_BUILD_READ_WRITE });
                task.UseImage({ .image = image, .access = AccessConsts::BLIT_WRITE }); // this is retarded fix
            },
            [this](ICommandBuffer* commands) {
                // Prepare RHI build infos
                BlasTriangleGeometryInfo triGeo{};
                triGeo.vertexFormat = Format::RGB32Sfloat;
                triGeo.indexType = IndexType::Uint32;
                triGeo.vertexBuffer = vertexBuffer->Internal();
                triGeo.indexBuffer = indexBuffer->Internal();
                triGeo.vertexStride = sizeof(float) * 3;
                triGeo.vertexCount = 3;
                triGeo.indexCount = 3;

                eastl::array<BlasTriangleGeometryInfo, 1> geoArray = { triGeo };
                BlasBuildInfo blasBuildInfo = { .geometries = geoArray };
                blasBuildInfo.dstBlas = blas->Internal();
                blasBuildInfo.scratchBuffer = blasScratchBuffer->Internal();

                TlasInstanceInfo tlasInstanceInfo = { .data = instanceBuffer->Internal(), .count = 1 };
                TlasBuildInfo tlasBuildInfo = { .instances = tlasInstanceInfo };
                tlasBuildInfo.dstTlas = tlas->Internal();
                tlasBuildInfo.scratchBuffer = tlasScratchBuffer->Internal();

                BuildAccelerationStructuresInfo buildAllInfo = {
                    .tlasBuildInfos = eastl::span<const TlasBuildInfo>(&tlasBuildInfo, 1),
                    .blasBuildInfos = eastl::span<const BlasBuildInfo>(&blasBuildInfo, 1),
                };

                commands->BuildAccelerationStructures(buildAllInfo);
            },
            TaskType::Transfer);

        // Task 1: Dispatch compute shader which performs ray queries
        GenericTask* dispatchTask = new ComputeCallbackTask(
            { .name = "RayQuery Compute Dispatch", .color = LabelColor::YELLOW },
            [this](ComputeTask& task) {
                task.UseImage({ .image = image, .access = AccessConsts::COMPUTE_SHADER_WRITE });
                task.UseTlas({ .tlas = tlas, .access = AccessConsts::ACCELERATION_STRUCTURE_BUILD_READ });
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

        // Task 2: Blit compute image to a full-size blit image that will be presented
        // GenericTask* blitTask = new CustomCallbackTask(
        //     { .name = "Blit To Swapchain Image", .color = LabelColor::YELLOW },
        //     [this](CustomTask& task) {
        //         task.UseImage({ .image = image,
        //             .access = AccessConsts::BLIT_READ });
        //         task.UseImage({ .image = blitImage,
        //             .access = AccessConsts::BLIT_WRITE });
        //     },
        //     [this](ICommandBuffer* commands) {
        //         BlitImageToImageInfo blitInfo{};
        //         blitInfo.srcImage = image->Internal();
        //         blitInfo.dstImage = blitImage->Internal();

        //         Extent3D srcDim = image->Info().size;
        //         Extent3D dstDim = blitImage->Info().size;

        //         blitInfo.srcImageBox = Box3D::Cut({ srcDim.width, srcDim.height, 1 });
        //         blitInfo.dstImageBox = Box3D::Cut({ dstDim.width, dstDim.height, 1 });
        //         blitInfo.filter = Filter::Nearest;

        //         commands->BlitImageToImage(blitInfo);
        //     },
        //     TaskType::Graphics);

    tasks = { buildTask, dispatchTask };

        return tasks;
    }

} // namespace VisualTests
