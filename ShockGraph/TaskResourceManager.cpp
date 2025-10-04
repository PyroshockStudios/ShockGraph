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

#include <PyroCommon/Logger.hpp>
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



        SHOCKGRAPH_API TaskResourceManager::TaskResourceManager(const TaskResourceManagerInfo& info)
            : mDevice(info.device), mRHI(info.rhi), mFramesInFlight(info.framesInFlight), mShaderReloadListener(new ShaderReloadListener(this)) {
            ASSERT(mRHI, "RHI was not set!");
            ASSERT(mDevice, "Device was not set!");
            ASSERT(mFramesInFlight >= 2, "Frames in flight must be at least 2!");
        }
        SHOCKGRAPH_API TaskResourceManager::~TaskResourceManager() {
            if (mResources.size() != mTombstones.size()) {
                Logger::Fatal(mLogStream, "Not all resources have been released before task resource manager destruction! "
                                          "All resources must be destroyed before the resource manager!");
            }
            delete mShaderReloadListener;
        }
        SHOCKGRAPH_API TaskBuffer TaskResourceManager::CreatePersistentBuffer(const TaskBufferInfo& info, eastl::span<const u8> initialData) {
            BufferUsageFlags extraRequiredFlags = {};
            eastl::vector<Buffer> buffersInFlight{};
            Buffer buffer = PYRO_NULL_BUFFER;
            ASSERT(!(!info.bCpuVisible && info.bReadback), "Readback buffers MUST be ");
            if (!initialData.empty()) {
                extraRequiredFlags |= BufferUsageFlagBits::TRANSFER_DST;
            }
            if (info.bDynamic) {
                buffersInFlight.resize(mFramesInFlight);
                for (i32 i = 0; i < mFramesInFlight; ++i) {
                    buffersInFlight[i] = mDevice->CreateBuffer({
                        .size = info.size,
                        .usage = BufferUsageFlagBits::TRANSFER_SRC | BufferUsageFlagBits::HOST_WRITE,
                        .initialLayout = info.bReadback ? BufferLayout::TransferDst : BufferLayout::TransferSrc,
                        .allocationDomain = info.bReadback ? MemoryAllocationDomain::HostReadback : MemoryAllocationDomain::HostRandomWrite,
                        .name = info.name + " (In Flight #" + eastl::to_string(i) + ")",
                    });
                }
            }
            if (info.bDynamic && info.bCpuVisible) {
                // No need to duplicate the buffers, can read write anyway
                buffer = buffersInFlight[0];
            } else {
                buffer = mDevice->CreateBuffer({
                    .size = info.size,
                    .usage = info.usage,
                    .initialLayout = info.bCpuVisible ? (info.bReadback ? BufferLayout::TransferDst : BufferLayout::ReadOnly) : BufferLayout::Undefined,
                    .allocationDomain = info.bCpuVisible ? (info.bReadback ? MemoryAllocationDomain::HostReadback : MemoryAllocationDomain::HostRandomWrite) : MemoryAllocationDomain::DeviceLocal,
                    .name = info.name,
                });
            }

            if (!initialData.empty()) {
                ASSERT(!info.bDynamic, "Cannot initialise a dynamic buffer with data!");
                ASSERT(!info.bReadback, "Cannot initialise a readback buffer with data!");
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
                mPendingStagingUploads.emplace_back(eastl::move(uploadPair));
            }

            TaskBuffer retBuffer = TaskBuffer::Create(this, info, eastl::move(buffer), eastl::move(buffersInFlight));
            if (info.bDynamic) {
                mDynamicBuffers.emplace_back(retBuffer.Get());
            }
            return retBuffer;
        }
        SHOCKGRAPH_API TaskImage TaskResourceManager::CreatePersistentImage(const TaskImageInfo& info, eastl::span<const u8> initialData) {
            ImageUsageFlags extraRequiredFlags = {};

            if (!initialData.empty()) {
                extraRequiredFlags |= ImageUsageFlagBits::TRANSFER_DST;
            }
            Image image = mDevice->CreateImage({
                .dimensions = info.size.z > 1u ? 3u : (info.size.y > 1u ? 2u : 1u),
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
                const u32 rowAlignment = mDevice->GetProperties().bufferImageRowAlignment;
                const DeviceSize minReqSize = RHIUtil::GetRequiredStagingSize(info.format, info.size.x, info.size.y, info.size.z, 1);
                ASSERT(minReqSize > 0, "Invalid format for staging upload");
                ASSERT(initialData.size_bytes() >= minReqSize, "Initial data is too small in size!");

                Buffer staging = mDevice->CreateBuffer({
                    .size = mDevice->ImageSizeRequirements(image),
                    .usage = BufferUsageFlagBits::TRANSFER_SRC,
                    .initialLayout = BufferLayout::TransferSrc,
                    .allocationDomain = MemoryAllocationDomain::HostStaging,
                    .name = info.name + " (Staging Buffer)",
                });

                const u32 rowWidth = static_cast<u32>(minReqSize / info.size.z / info.size.y);
                const u32 rowPitch = mDevice->ImageSubresourceRowPitch(image, {}, rowWidth);

                u8* dstPtr = mDevice->BufferHostAddress(staging);
                const u8* srcPtr = initialData.data();
                RHIUtil::CopyAlignedTextureData(srcPtr, dstPtr, rowWidth, info.size.y, info.size.z, rowPitch);

                StagingUploadPair uploadPair{};
                uploadPair.srcBuffer = staging;

                // push one upload per mip level
                for (u32 mip = 0; mip < info.mipLevelCount; ++mip) {
                    uploadPair.uploads.push_back({ .dstImage = image,
                        .dstImageLayout = ImageLayout::ReadOnly,
                        .dstImageSlice = {
                            .mipLevel = mip,
                            .baseArrayLayer = 0,
                            .layerCount = info.arrayLayerCount,
                        },
                        .rowPitch = static_cast<u32>(rowPitch) });
                }

                mPendingStagingUploads.emplace_back(eastl::move(uploadPair));
            }
            return TaskImage::Create(this, info, eastl::move(image));
        }
        SHOCKGRAPH_API ShaderResourceId TaskResourceManager::DefaultShaderResourceView(TaskImage image) {
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
                case 1:
                    resourceInfo.viewType = resourceInfo.slice.layerCount > 1 ? ImageViewType::e1DArray : ImageViewType::e1D;
                    break;
                case 2:
                    resourceInfo.viewType = resourceInfo.slice.layerCount > 1 ? ImageViewType::e2DArray : ImageViewType::e2D;
                    break;
                case 3:
                    resourceInfo.viewType = ImageViewType::e3D;
                    break;
                };
            }
            return mDevice->CreateShaderResource(resourceInfo);
        }
        SHOCKGRAPH_API ShaderResourceId TaskResourceManager::DefaultShaderResourceView(TaskBuffer buffer) {
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

        SHOCKGRAPH_API TaskColorTarget TaskResourceManager::CreateColorTarget(const TaskColorTargetInfo& info) {
            ASSERT(info.image, "No Image defined!");
            RenderTarget renderTarget = mDevice->CreateRenderTarget({
                .image = info.image->Internal(),
                .slice = info.slice,
                .flags = RenderTargetFlagBits::COLOR_TARGET,
                .name = info.name,
            });
            return TaskColorTarget::Create(this, info, eastl::move(renderTarget));
        }
        SHOCKGRAPH_API TaskDepthStencilTarget TaskResourceManager::CreateDepthStencilTarget(const TaskDepthStencilTargetInfo& info) {
            ASSERT(info.image, "No Image defined!");
            RenderTarget renderTarget = mDevice->CreateRenderTarget({
                .image = info.image->Internal(),
                .slice = info.slice,
                .flags = (info.bDepth ? RenderTargetFlagBits::DEPTH_TARGET : RenderTargetFlags{}) | (info.bStencil ? RenderTargetFlagBits::STENCIL_TARGET : RenderTargetFlags{}),
                .name = info.name,
            });
            return TaskDepthStencilTarget::Create(this, info, eastl::move(renderTarget));
        }
        SHOCKGRAPH_API ShaderResourceId TaskResourceManager::CreateShaderResourceView(const TaskBufferResourceInfo& info) {
            return mDevice->CreateShaderResource(BufferResourceInfo{
                .buffer = info.buffer->Internal(),
                .region = info.region,
            });
        }
        SHOCKGRAPH_API ShaderResourceId TaskResourceManager::CreateShaderResourceView(const TaskImageResourceInfo& info) {
            return mDevice->CreateShaderResource(ImageResourceInfo{
                .image = info.image->Internal(),
                .slice = info.slice,
                .viewType = info.viewType,
                .format = info.format,
            });
        }
        SHOCKGRAPH_API UnorderedAccessId TaskResourceManager::CreateUnorderedAccessView(const TaskBufferResourceInfo& info) {
            return mDevice->CreateUnorderedAccess(BufferResourceInfo{
                .buffer = info.buffer->Internal(),
                .region = info.region,
            });
        }
        SHOCKGRAPH_API UnorderedAccessId TaskResourceManager::CreateUnorderedAccessView(const TaskImageResourceInfo& info) {
            return mDevice->CreateUnorderedAccess(ImageResourceInfo{
                .image = info.image->Internal(),
                .slice = info.slice,
                .viewType = info.viewType,
                .format = info.format,
            });
        }
        SHOCKGRAPH_API SamplerId TaskResourceManager::CreateSampler(const TaskSamplerInfo& info) {
            return mDevice->CreateSampler(info);
        }
        SHOCKGRAPH_API void TaskResourceManager::ReleaseShaderResourceView(ShaderResourceId& id) {
            mDevice->Destroy(id);
        }
        SHOCKGRAPH_API void TaskResourceManager::ReleaseUnorderedAccessView(UnorderedAccessId& id) {
            mDevice->Destroy(id);
        }
        SHOCKGRAPH_API void TaskResourceManager::ReleaseSampler(SamplerId& id) {
            mDevice->Destroy(id);
        }

        SHOCKGRAPH_API TaskRasterPipeline TaskResourceManager::CreateRasterPipeline(const TaskRasterPipelineInfo& info, const TaskRasterPipelineShaders& shaders) {
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
        SHOCKGRAPH_API TaskComputePipeline TaskResourceManager::CreateComputePipeline(const TaskComputePipelineInfo& info, const TaskShaderInfo& shader) {
            TaskComputePipeline pipeline = TaskComputePipeline::Create(this, info, shader);
            shader.program->mUsedByResources.emplace_back(pipeline.Get());
            pipeline->Recreate();
            return pipeline;
        }
        SHOCKGRAPH_API TaskSwapChain TaskResourceManager::CreateSwapChain(const TaskSwapChainInfo& info) {
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
                .presentMode = info.vsync ? PresentMode::VSync : PresentMode::LowLatency,
                .bufferCount = mFramesInFlight,
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
        void TaskResourceManager::RegisterResource(TaskResource_* resource) {
            resource->mOwner = this;
            if (mTombstones.empty()) {
                resource->mId = static_cast<u32>(mResources.size());
                mResources.emplace_back(resource);
            } else {
                resource->mId = mTombstones.back();
                mTombstones.pop_back();
                mResources[resource->mId] = resource;
            }
        }
        void TaskResourceManager::ReleaseResource(TaskResource_* resource) {
            u32 slot = resource->GetId();
            ASSERT(mResources.size() > slot, "Bad slot!");
            ASSERT(mResources[slot], "Double delete!");
            ASSERT(eastl::find(mTombstones.begin(), mTombstones.end(), slot) == mTombstones.end(), "Double delete!");
            mTombstones.emplace_back(slot);
            mResources[slot] = {};
        }
        void TaskResourceManager::ReleaseBufferResource(TaskBuffer_* resource) {
            if (resource->Info().bDynamic) {
                auto it = eastl::find(mDynamicBuffers.begin(), mDynamicBuffers.end(), resource);
                ASSERT(it != mDynamicBuffers.end());
                mDynamicBuffers.erase(it);
            }
            for (auto& staging : mPendingStagingUploads) {
                for (i32 i = 0; i < staging.uploads.size(); ++i) {
                    auto& upload = staging.uploads[i];
                    if (upload.dstBuffer == resource->Internal()) {
                        staging.uploads.erase(staging.uploads.begin() + i);
                        --i;
                    }
                }
            }
        }
        void TaskResourceManager::ReleaseImageResource(TaskImage_* resource) {
            for (auto& staging : mPendingStagingUploads) {
                for (i32 i = 0; i < staging.uploads.size(); ++i) {
                    auto& upload = staging.uploads[i];
                    if (upload.dstImage == resource->Internal()) {
                        staging.uploads.erase(staging.uploads.begin() + i);
                        --i;
                    }
                }
            }
        }
    } // namespace Renderer
} // namespace PyroshockStudios