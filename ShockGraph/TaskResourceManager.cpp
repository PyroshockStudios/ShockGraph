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

#include "TaskResourceManager.hpp"
#include "IShaderReloadListener.hpp"

#ifdef SHOCKGRAPH_USE_PYRO_PLATFORM
#include <PyroPlatform/Window/IWindow.hpp>
#endif
#include <PyroRHI/Api/IDevice.hpp>
#include <PyroRHI/Api/Util.hpp>
#include <PyroRHI/Context.hpp>

#include <EASTL/shared_ptr.h>
#include <PyroCommon/Logger.hpp>
#include <PyroRHI/Common/AtomicMap.hpp>
#include <libassert/assert.hpp>

namespace PyroshockStudios {
    inline namespace Renderer {
        class ShaderReloadListener : public IShaderReloadListener {
        public:
            ShaderReloadListener(TaskResourceManager* self) {
            }

            void OnShaderChange(TaskShaderHandle shader, ShaderProgram&& newProgram) override {
                shader->mProgram = eastl::move(newProgram);
                for (TaskResource_* resource : shader->mUsedByResources) {
                    if (auto* rpipeline = dynamic_cast<TaskRasterPipeline_*>(resource); rpipeline) {
                        rpipeline->mbDirty = true;
                    } else if (auto* cpipeline = dynamic_cast<TaskComputePipeline_*>(resource); cpipeline) {
                        cpipeline->mbDirty = true;
                    } else {
                        ASSERT(false, "Bad resource reference! Expected compute shader or raster pipeline!");
                    }
                }
            }

            TaskResourceManager* self = nullptr;
        };

        using ResourceStates = Common::AtomicMap<IDevice*, eastl::shared_ptr<TaskResourceManager::ResourceStateMap>>;
        static ResourceStates& GetResourceStates() {
            static ResourceStates gResourceStates;
            return gResourceStates;
        }

        TaskResourceManager::TaskResourceManager(const TaskResourceManagerInfo& info)
            : mDevice(info.device), mRHI(info.rhi), mFramesInFlight(info.framesInFlight), mShaderReloadListener(new ShaderReloadListener(this)) {
            ASSERT(mRHI, "RHI was not set!");
            ASSERT(mDevice, "Device was not set!");

            {
                std::lock_guard l(GetResourceStates().GetLock());
                if (!GetResourceStates().UnderlyingMap().contains(mDevice)) {
                    GetResourceStates().UnderlyingMap().emplace(mDevice, eastl::make_shared<TaskResourceManager::ResourceStateMap>());
                }
            }
        }

        TaskResourceManager::~TaskResourceManager() {
            if (mResources.Size() != mTombstones.Size()) {
                Logger::Fatal(mLogStream, "Not all resources have been released before task resource manager destruction! "
                                          "All resources must be destroyed before the resource manager!");
            }
            delete mShaderReloadListener;
        }

        TaskBuffer TaskResourceManager::CreatePersistentBuffer(const TaskBufferInfo& info, eastl::span<const u8> initialData) {
            ASSERT(!bool(info.usage & BufferUsageFlagBits::UNIFORM_BUFFER) || info.size <= Limits::MAX_UNIFORM_BUFFER_SIZE, "Ubos must be at most UINT16 bytes in size!");
            BufferUsageFlags extraRequiredFlags = {};
            eastl::vector<Buffer> buffersInFlight{};
            Buffer buffer = PYRO_NULL_BUFFER;
            if (!initialData.empty()) {
                extraRequiredFlags |= BufferUsageFlagBits::TRANSFER_DST;
            }
            bool bExposeFlightBuffers = info.mode == TaskBufferMode::HostDynamic || info.mode == TaskBufferMode::Readback;
            if (info.mode == TaskBufferMode::Dynamic || bExposeFlightBuffers) {
                buffersInFlight.resize(mFramesInFlight);
                for (u32 i = 0; i < mFramesInFlight; ++i) {
                    buffersInFlight[i] = mDevice->CreateBuffer({
                        .size = info.size,
                        .usage = (bExposeFlightBuffers ? (info.usage) : (BufferUsageFlagBits::TRANSFER_SRC)) | extraRequiredFlags,
                        .initialLayout = info.mode == TaskBufferMode::Readback ? BufferLayout::TransferDst : BufferLayout::TransferSrc,
                        .allocationDomain = info.mode == TaskBufferMode::Readback ? MemoryAllocationDomain::HostReadback : MemoryAllocationDomain::HostRandomWrite,
                        .name = info.name + " (In Flight #" + eastl::to_string(i) + ")",
                    });
                }
            }
            if (info.mode == TaskBufferMode::Readback || info.mode == TaskBufferMode::HostDynamic) {
                // No need to duplicate the buffers, can read write anyway
                buffer = buffersInFlight[0];
            } else if (info.mode == TaskBufferMode::Default) {
                buffer = mDevice->CreateBuffer({
                    .size = info.size,
                    .usage = info.usage | extraRequiredFlags,
                    .initialLayout = BufferLayout::Undefined,
                    .allocationDomain = MemoryAllocationDomain::DeviceLocal,
                    .name = info.name,
                });
            } else if (info.mode == TaskBufferMode::Host) {
                buffer = mDevice->CreateBuffer({
                    .size = info.size,
                    .usage = info.usage | extraRequiredFlags,
                    .initialLayout = BufferLayout::ReadOnly,
                    .allocationDomain = MemoryAllocationDomain::HostRandomWrite,
                    .name = info.name,
                });
            } else if (info.mode == TaskBufferMode::Dynamic) {
                buffer = mDevice->CreateBuffer({
                    .size = info.size,
                    .usage = info.usage | extraRequiredFlags,
                    .initialLayout = BufferLayout::TransferDst,
                    .allocationDomain = MemoryAllocationDomain::DeviceLocal,
                    .name = info.name,
                });
            } else {
                ASSERT("Bad buffer mode!");
            }

            if (!initialData.empty()) {
                ASSERT(info.mode == TaskBufferMode::Default, "Only buffers with Default mode can be initialised with data!");
                ASSERT(initialData.size_bytes() >= info.size, "Initial data is too small in size!");
                Buffer staging = mDevice->CreateBuffer({
                    .size = info.size,
                    .usage = BufferUsageFlagBits::TRANSFER_SRC,
                    .initialLayout = BufferLayout::TransferSrc,
                    .allocationDomain = MemoryAllocationDomain::HostStaging,
                    .name = info.name + " (Staging Buffer)",
                });
                memcpy(mDevice->BufferHostAddress(staging), initialData.data(), info.size);

                StagingUploadPair uploadPair{};
                uploadPair.srcBuffer = staging;
                uploadPair.uploads.push_back({
                    .dstBuffer = buffer,
                    .dstBufferLayout = BufferLayout::ReadOnly,
                });
                mPendingStagingUploads.EmplaceBack(eastl::move(uploadPair));
            }

            if (info.mode == TaskBufferMode::Host) {
                buffersInFlight.resize(mFramesInFlight, buffer);
            }
            TaskBuffer retBuffer = TaskBuffer::Create(this, info, eastl::move(buffer), eastl::move(buffersInFlight));
            if (info.mode == TaskBufferMode::Dynamic || info.mode == TaskBufferMode::HostDynamic || info.mode == TaskBufferMode::Readback) {
                mDynamicBuffers.EmplaceBack(retBuffer.Get());
            }
            return retBuffer;
        }

        TaskImage TaskResourceManager::CreatePersistentImage(const TaskImageInfo& info, eastl::span<const u8> initialData) {
            ImageUsageFlags extraRequiredFlags = {};

            if (!initialData.empty()) {
                extraRequiredFlags |= ImageUsageFlagBits::TRANSFER_DST;
            }
            Image image = mDevice->CreateImage({
                .flags = info.flags,
                .dimensions = info.dimensions,
                .format = info.format,
                .size = info.size,
                .mipLevelCount = info.mipLevelCount,
                .arrayLayerCount = info.arrayLayerCount,
                .sampleCount = info.sampleCount,
                .usage = info.usage | extraRequiredFlags,
                .name = info.name,
            });
            // FIXME, texture arrays/mipmaps!
            if (!initialData.empty()) {
                const u32 rowAlignment = mDevice->Properties().bufferImageRowAlignment;

                const RHIUtil::FormatBlockInfo blockInfo = RHIUtil::GetFormatBlockInfo(info.format);

                const u32 blocksX = (info.size.width + blockInfo.blockWidth - 1) / blockInfo.blockWidth;
                const u32 blocksY = (info.size.height + blockInfo.blockHeight - 1) / blockInfo.blockHeight;

                const u32 tightRowPitch = blocksX * blockInfo.bytesPerBlock;
                const u32 alignedRowPitch = PYRO_ALIGN(tightRowPitch, rowAlignment);
                const DeviceSize stagingSize = alignedRowPitch * blocksY * info.size.depth;

                const DeviceSize tightSize = tightRowPitch * blocksY * info.size.depth;
                ASSERT(initialData.size_bytes() >= tightSize, "Initial data is too small!");

                Buffer staging = mDevice->CreateBuffer({
                    .size = stagingSize, // Use calculated staging size, not ImageRequirements (which might include mips)
                    .usage = BufferUsageFlagBits::TRANSFER_SRC,
                    .initialLayout = BufferLayout::TransferSrc,
                    .allocationDomain = MemoryAllocationDomain::HostStaging,
                    .name = info.name + " (Staging Buffer)",
                });

                u8* dstPtr = mDevice->BufferHostAddress(staging);
                const u8* srcPtr = initialData.data();

                RHIUtil::CopyAlignedTextureData(srcPtr, dstPtr, tightRowPitch, blocksY, info.size.depth, alignedRowPitch);

                StagingUploadPair uploadPair{};
                uploadPair.srcBuffer = staging;

                // FIXME: Only uploading Mip 0 for now.
                // To support all mips, you must loop mips, recalculate blocksX/Y per mip,
                // and offset srcPtr and dstPtr accordingly.
                uploadPair.uploads.push_back({ .dstImage = image,
                    .dstImageLayout = ImageLayout::ReadOnly,
                    .dstImageSlice = {
                        .mipLevel = 0,
                        .baseArrayLayer = 0,
                        .layerCount = info.arrayLayerCount,
                    },
                    .rowPitch = alignedRowPitch });

                mPendingStagingUploads.EmplaceBack(eastl::move(uploadPair));
            }
            return TaskImage::Create(this, info, eastl::move(image));
        }

        TaskBlas TaskResourceManager::CreatePersistentBlas(const TaskBlasInfo& info) {
            BlasId blas = mDevice->CreateBlas({
                .size = info.size,
                .name = info.name,
            });

            return TaskBlas::Create(this, info, eastl::move(blas));
        }

        TaskTlas TaskResourceManager::CreatePersistentTlas(const TaskTlasInfo& info) {
            TlasId tlas = mDevice->CreateTlas({
                .size = info.size,
                .name = info.name,
            });

            return TaskTlas::Create(this, info, eastl::move(tlas));
        }

        ShaderResourceId TaskResourceManager::DefaultShaderResourceView(TaskImage image) {
            auto& imageInfo = mDevice->GetImageInfo(image->Internal());
            ImageResourceInfo resourceInfo{
                .image = image->Internal(),
                .slice = {
                    .levelCount = imageInfo.mipLevelCount,
                    .layerCount = imageInfo.arrayLayerCount,
                },
                .format = imageInfo.format,
            };
            if (imageInfo.flags & ImageCreateFlagBits::CUBE) {
                resourceInfo.viewType = resourceInfo.slice.layerCount > 1 ? ImageViewType::eCubeArray : ImageViewType::eCube;
            } else {
                switch (imageInfo.dimensions) {
                case ImageDimensions::e1D:
                    resourceInfo.viewType = resourceInfo.slice.layerCount > 1 ? ImageViewType::e1DArray : ImageViewType::e1D;
                    break;
                case ImageDimensions::e2D:
                    resourceInfo.viewType = resourceInfo.slice.layerCount > 1 ? ImageViewType::e2DArray : ImageViewType::e2D;
                    break;
                case ImageDimensions::e3D:
                    resourceInfo.viewType = ImageViewType::e3D;
                    break;
                };
            }
            return mDevice->CreateShaderResource(resourceInfo);
        }

        ShaderResourceId TaskResourceManager::DefaultShaderResourceView(TaskBuffer buffer) {
            auto& bufferInfo = mDevice->GetBufferInfo(buffer->Internal());
            BufferResourceInfo resourceInfo{
                .buffer = buffer->Internal(),
                .region = {
                    .offset = 0,
                    .size = bufferInfo.size,
                },
            };
            return mDevice->CreateShaderResource(resourceInfo);
        }

        TaskColorTarget TaskResourceManager::CreateColorTarget(const TaskColorTargetInfo& info) {
            ASSERT(info.image, "No Image defined!");
            RenderTarget renderTarget = mDevice->CreateRenderTarget({
                .image = info.image->Internal(),
                .slice = info.slice,
                .flags = RenderTargetFlagBits::COLOR_TARGET,
                .name = info.name,
            });
            return TaskColorTarget::Create(this, info, eastl::move(renderTarget));
        }

        TaskDepthStencilTarget TaskResourceManager::CreateDepthStencilTarget(const TaskDepthStencilTargetInfo& info) {
            ASSERT(info.image, "No Image defined!");
            ASSERT(info.bDepth || info.bStencil, "Must define at least depth or stencil target usage!!");
            RenderTarget renderTarget = mDevice->CreateRenderTarget({
                .image = info.image->Internal(),
                .slice = info.slice,
                .flags = (info.bDepth ? RenderTargetFlagBits::DEPTH_TARGET : RenderTargetFlags{}) | (info.bStencil ? RenderTargetFlagBits::STENCIL_TARGET : RenderTargetFlags{}),
                .name = info.name,
            });
            return TaskDepthStencilTarget::Create(this, info, eastl::move(renderTarget));
        }

        eastl::pair<TaskColorTarget, TaskImage> TaskResourceManager::CreateColorTargetAndImage(const TaskImageInfo& info) {
            ASSERT(info.arrayLayerCount == 1 && info.mipLevelCount == 1, "Do not call CreateColorTargetAndImage to make complex images!");
            ASSERT(info.dimensions == ImageDimensions::e2D, "CreateColorTargetAndImage only supports 2D images!");
            TaskImage img = CreatePersistentImage(info);
            return {
                CreateColorTarget({ .image = img, .name = info.name.empty() ? "" : info.name + " (Render Target)" }), img
            };
        }
        eastl::pair<TaskDepthStencilTarget, TaskImage> TaskResourceManager::CreateDepthStencilTargetAndImage(const TaskImageInfo& info, bool bDepth, bool bStencil) {
            ASSERT(info.arrayLayerCount == 1 && info.mipLevelCount == 1, "Do not call CreateDepthStencilTargetAndImage to make complex images!");
            ASSERT(info.dimensions == ImageDimensions::e2D, "CreateDepthStencilTargetAndImage only supports 2D images!");
            TaskImage img = CreatePersistentImage(info);
            return {
                CreateDepthStencilTarget({ .image = img, .bDepth = bDepth, .bStencil = bStencil, .name = info.name.empty() ? "" : info.name + " (Render Target)" }), img
            };
        }


        ShaderResourceId TaskResourceManager::CreateShaderResourceView(const TaskBufferResourceInfo& info) {
            return mDevice->CreateShaderResource(BufferResourceInfo{
                .buffer = info.buffer->Internal(),
                .region = info.region,
            });
        }

        ShaderResourceId TaskResourceManager::CreateShaderResourceView(const TaskImageResourceInfo& info) {
            return mDevice->CreateShaderResource(ImageResourceInfo{
                .image = info.image->Internal(),
                .slice = info.slice,
                .viewType = info.viewType,
                .format = info.format,
            });
        }

        UnorderedAccessId TaskResourceManager::CreateUnorderedAccessView(const TaskBufferResourceInfo& info) {
            return mDevice->CreateUnorderedAccess(BufferResourceInfo{
                .buffer = info.buffer->Internal(),
                .region = info.region,
            });
        }

        UnorderedAccessId TaskResourceManager::CreateUnorderedAccessView(const TaskImageResourceInfo& info) {
            return mDevice->CreateUnorderedAccess(ImageResourceInfo{
                .image = info.image->Internal(),
                .slice = info.slice,
                .viewType = info.viewType,
                .format = info.format,
            });
        }

        SamplerId TaskResourceManager::CreateSampler(const TaskSamplerInfo& info) {
            return mDevice->CreateSampler(info);
        }

        void TaskResourceManager::ReleaseShaderResourceView(ShaderResourceId& id) {
            mDevice->DestroyDeferred(id);
            id = PYRO_NULL_SRV;
        }

        void TaskResourceManager::ReleaseUnorderedAccessView(UnorderedAccessId& id) {
            mDevice->DestroyDeferred(id);
            id = PYRO_NULL_UAV;
        }

        void TaskResourceManager::ReleaseSampler(SamplerId& id) {
            mDevice->DestroyDeferred(id);
            id = PYRO_NULL_SAMPLER;
        }

        TaskRasterPipeline TaskResourceManager::CreateRasterPipeline(const TaskRasterPipelineInfo& info, const TaskRasterPipelineShaders& shaders) {
            eastl::vector<TaskShader_*> addRefs = {};

            if (shaders.vertexShaderInfo) {
                auto& v = shaders.vertexShaderInfo.value();
                ASSERT(v.program, "Do not pass null programs into vertex shader info!");
                addRefs.emplace_back(v.program.Get());
            }
            if (shaders.domainShaderInfo) {
                auto& v = shaders.domainShaderInfo.value();
                ASSERT(v.program, "Do not pass null programs into domain shader info!");
                addRefs.emplace_back(v.program.Get());
            }
            if (shaders.hullShaderInfo) {
                auto& v = shaders.hullShaderInfo.value();
                ASSERT(v.program, "Do not pass null programs into hull shader info!");
                addRefs.emplace_back(v.program.Get());
            }
            if (shaders.geometryShaderInfo) {
                auto& v = shaders.geometryShaderInfo.value();
                ASSERT(v.program, "Do not pass null programs into geometry shader info!");
                addRefs.emplace_back(v.program.Get());
            }
            if (shaders.fragmentShaderInfo) {
                auto& v = shaders.fragmentShaderInfo.value();
                ASSERT(v.program, "Do not pass null programs into fragment shader info!");
                addRefs.emplace_back(v.program.Get());
            }

            TaskRasterPipeline pipeline = TaskRasterPipeline::Create(this, info, shaders);
            for (TaskShader_* shader : addRefs) {
                shader->mUsedByResources.emplace_back(pipeline.Get());
            }
            pipeline->Recreate();
            return pipeline;
        }

        TaskComputePipeline TaskResourceManager::CreateComputePipeline(const TaskComputePipelineInfo& info, const TaskShaderInfo& shader) {
            TaskComputePipeline pipeline = TaskComputePipeline::Create(this, info, shader);
            shader.program->mUsedByResources.emplace_back(pipeline.Get());
            pipeline->Recreate();
            return pipeline;
        }

        TaskSwapChain TaskResourceManager::CreateSwapChain(const TaskSwapChainInfo& info) {
            SwapChainFormat format = {};
            switch (info.format) {
            case TaskSwapChainFormat::e8Bit:
                format = SwapChainFormat::Unorm8BitLDR;
                break;
            case TaskSwapChainFormat::e10Bit:
                format = SwapChainFormat::Unorm10BitLDR;
                break;
            case TaskSwapChainFormat::e16BitHDR:
                format = SwapChainFormat::Float16BitHDR;
                break;
            default:
                ASSERT(false, "unsupported format");
                break;
            }

            ISwapChain* swapChain = mDevice->CreateSwapChain({
#ifdef SHOCKGRAPH_USE_PYRO_PLATFORM
                .nativeWindow = info.window->GetNativeWindow(),
                .nativeInstance = info.window->GetNativeInstance(),
#else
                .nativeWindow = info.nativeWindow,
                .nativeInstance = info.nativeInstance,
#endif
                .format = format,
                .presentMode = info.vsync ? SwapChainPresentMode::VSync : SwapChainPresentMode::LowLatency,
                .bufferCount = info.bufferCount == TASK_SWAP_CHAIN_AUTO_BUFFERS ? mFramesInFlight + 1 : info.bufferCount,
                .imageUsage = info.imageUsage,
#ifdef SHOCKGRAPH_USE_PYRO_PLATFORM
                .extent = { info.window->GetSize().width, info.window->GetSize().height },
#else
                .extent = info.nativeWindowExtent,
#endif
                .name = info.name,
            });
            return TaskSwapChain::Create(this, info, eastl::move(swapChain));
        }

        void TaskResourceManager::SetFramesInFlight(u32 newFramesInFlight) {
            ASSERT(false, "Not implemented");
        }

        IShaderReloadListener* TaskResourceManager::GetShaderReloadListener() {
            return mShaderReloadListener;
        }

        TaskResourceManager::ResourceStateMap& TaskResourceManager::GetResourceStateMap() {
            return *GetResourceStates().Get(mDevice);
        }
        void TaskResourceManager::RegisterResource(TaskResource_* resource) {
            resource->mOwner = this;
            if (mTombstones.Empty()) {
                resource->mId = static_cast<u32>(mResources.Size());
                mResources.EmplaceBack(resource);
            } else {
                resource->mId = mTombstones.PopBack();
                mResources.Assign(resource->mId, resource);
            }
        }

        void TaskResourceManager::ReleaseResource(TaskResource_* resource) {
            u32 slot = resource->GetId();
            ASSERT(mResources.Size() > slot, "Bad slot!");
            ASSERT(mResources.At(slot) != nullptr, "Double delete!");
            ASSERT(!mTombstones.Contains(slot), "Double delete!");
            mTombstones.EmplaceBack(slot);
            mResources.Assign(slot, nullptr);
        }

        void TaskResourceManager::ReleaseBufferResource(TaskBuffer_* resource) {
            auto& states = GetResourceStateMap();
            const auto& info = resource->Info();
            if (info.mode == TaskBufferMode::Dynamic || info.mode == TaskBufferMode::HostDynamic || info.mode == TaskBufferMode::Readback) {
                ASSERT(mDynamicBuffers.Contains(resource));
                mDynamicBuffers.EraseFirstItem(resource);
                for (Buffer buffer : resource->mInFlightBuffers) {
                    auto it = states.mLastKnownBufferLayouts.find(buffer);
                    if (it != states.mLastKnownBufferLayouts.end()) {
                        states.mLastKnownBufferLayouts.erase(it);
                    }
                }
            } else {
                auto it = states.mLastKnownBufferLayouts.find(resource->Internal());
                if (it != states.mLastKnownBufferLayouts.end()) {
                    states.mLastKnownBufferLayouts.erase(it);
                }
            }
            {
                std::lock_guard l(mPendingStagingUploads.GetLock());
                auto& vec = mPendingStagingUploads.UnderlyingVector();
                for (auto& staging : vec) {
                    for (i32 i = 0; i < staging.uploads.size(); ++i) {
                        auto& upload = staging.uploads[i];
                        if (upload.dstBuffer == resource->Internal()) {
                            staging.uploads.erase(staging.uploads.begin() + i);
                            --i;
                        }
                    }
                }
            }
        }

        void TaskResourceManager::ReleaseImageResource(TaskImage_* resource) {
            auto& states = GetResourceStateMap();
            {
                std::lock_guard l(mPendingStagingUploads.GetLock());
                auto& vec = mPendingStagingUploads.UnderlyingVector();
                for (auto& staging : vec) {
                    for (i32 i = 0; i < staging.uploads.size(); ++i) {
                        auto& upload = staging.uploads[i];
                        if (upload.dstImage == resource->Internal()) {
                            staging.uploads.erase(staging.uploads.begin() + i);
                            --i;
                        }
                    }
                }
            }
            auto it = states.mLastKnownImageLayouts.find(resource->Internal());
            if (it != states.mLastKnownImageLayouts.end()) {
                states.mLastKnownImageLayouts.erase(it);
            }
        }
    } // namespace Renderer
} // namespace PyroshockStudios