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

#include "Resources.hpp"
#include "TaskResourceManager.hpp"
#include <PyroCommon/Core.hpp>
#include <PyroCommon/Memory.hpp>
#include <PyroRHI/Api/Forward.hpp>
#include <PyroRHI/Api/GPUResource.hpp>
#include <PyroRHI/Api/IDevice.hpp>
#include <PyroRHI/Api/Types.hpp>

#include <future>

namespace PyroshockStudios {
    inline namespace Renderer {
        TaskResource_ ::TaskResource_(TaskResourceManager* owner) : mOwner(owner) {
            mOwner->RegisterResource(this);
        }
        TaskResource_ ::~TaskResource_() {
            mOwner->ReleaseResource(this);
        }

        IDevice* TaskResource_::Device() {
            return mOwner->mDevice;
        }
        TaskShader_::TaskShader_(ShaderProgram&& program, FunctionPtr<void(void*, TaskShader_*)> deleter, void* deleterUserData)
            : mProgram(eastl::move(program)), mDeleter(eastl::move(deleter)), mDeleterUserData(deleterUserData) {}

        TaskShader_::~TaskShader_() { mDeleter(mDeleterUserData, this); }

        TaskRasterPipeline_::TaskRasterPipeline_(TaskResourceManager* owner, const TaskRasterPipelineInfo& info, const TaskRasterPipelineShaders& shaderStages)
            : TaskResource_(owner), mInfo(info), mStages(shaderStages) {
        }
        TaskRasterPipeline_::~TaskRasterPipeline_() {
            if (mStages.vertexShaderInfo) {
                mStages.vertexShaderInfo->program->RemoveReference(this);
            }
            if (mStages.domainShaderInfo) {
                mStages.domainShaderInfo->program->RemoveReference(this);
            }
            if (mStages.hullShaderInfo) {
                mStages.hullShaderInfo->program->RemoveReference(this);
            }
            if (mStages.geometryShaderInfo) {
                mStages.geometryShaderInfo->program->RemoveReference(this);
            }
            if (mStages.fragmentShaderInfo) {
                mStages.fragmentShaderInfo->program->RemoveReference(this);
            }
            Device()->DestroyDeferred(mPipeline);
        }
        void TaskRasterPipeline_::Recreate() {
            RasterPipelineShaderStages copyStages;
            copyStages.vertexShaderInfo = eastl::nullopt;
            copyStages.domainShaderInfo = eastl::nullopt;
            copyStages.hullShaderInfo = eastl::nullopt;
            copyStages.geometryShaderInfo = eastl::nullopt;
            copyStages.fragmentShaderInfo = eastl::nullopt;
            if (mStages.vertexShaderInfo) {
                auto& v = mStages.vertexShaderInfo.value();
                copyStages.vertexShaderInfo.emplace(ShaderInfo{
                    .program = v.program->Program().bytecode,
                    .specializationConstants = v.specializationConstants,
                });
            }
            if (mStages.domainShaderInfo) {
                auto& v = mStages.domainShaderInfo.value();
                copyStages.domainShaderInfo.emplace(ShaderInfo{
                    .program = v.program->Program().bytecode,
                    .specializationConstants = v.specializationConstants,
                });
            }
            if (mStages.hullShaderInfo) {
                auto& v = mStages.hullShaderInfo.value();
                copyStages.hullShaderInfo.emplace(ShaderInfo{
                    .program = v.program->Program().bytecode,
                    .specializationConstants = v.specializationConstants,
                });
            }
            if (mStages.geometryShaderInfo) {
                auto& v = mStages.geometryShaderInfo.value();
                copyStages.geometryShaderInfo.emplace(ShaderInfo{
                    .program = v.program->Program().bytecode,
                    .specializationConstants = v.specializationConstants,
                });
            }
            if (mStages.fragmentShaderInfo) {
                auto& v = mStages.fragmentShaderInfo.value();
                copyStages.fragmentShaderInfo.emplace(ShaderInfo{
                    .program = v.program->Program().bytecode,
                    .specializationConstants = v.specializationConstants,
                });
            }
            mPipeline = Device()->Create(mInfo, copyStages);
        }
        TaskComputePipeline_::TaskComputePipeline_(TaskResourceManager* owner, const TaskComputePipelineInfo& info, const TaskShaderInfo& shader)
            : TaskResource_(owner), mInfo(info), mShader(shader) {
        }
        TaskComputePipeline_::~TaskComputePipeline_() {
            mShader.program->RemoveReference(this);
            Device()->DestroyDeferred(mPipeline);
        }
        void TaskComputePipeline_::Recreate() {
            ShaderInfo copyShader;
            copyShader.program = mShader.program->Program().bytecode;
            copyShader.specializationConstants = mShader.specializationConstants;
            mPipeline = Device()->Create(mInfo, copyShader);
        }

        TaskBuffer_::TaskBuffer_(TaskResourceManager* owner, const TaskBufferInfo& info, Buffer&& buffer, eastl::vector<Buffer>&& inFlightBuffers)
            : TaskResource_(owner), mBuffer(buffer), mInFlightBuffers(eastl::move(inFlightBuffers)), mInfo(info) {
        }
        TaskBuffer_::~TaskBuffer_() {
            Owner()->ReleaseBufferResource(this);
            if (this->mInfo.mode == TaskBufferMode::HostDynamic || this->mInfo.mode == TaskBufferMode::Readback) {
                // Do not destroy mBuffer as it is the same stuff as in mInFlightBuffers!
            } else {
                Device()->DestroyDeferred(mBuffer);
            }
            if (this->mInfo.mode != TaskBufferMode::Host) { // Do not destroy these as they are copies of mBuffer!
                for (auto& buffer : mInFlightBuffers) {
                    Device()->DestroyDeferred(buffer);
                }
            }
        }
        void* TaskBuffer_::MapMemory(const BufferRegion& region) {
            return Device()->BufferHostAddress(InternalInFlightBuffer(mCurrentBufferInFlight)) + region.offset;
        }
        void TaskBuffer_::UnmapMemory(void* memory) {
        }
        TaskImage_::TaskImage_(TaskResourceManager* owner, const TaskImageInfo& info, Image&& image)
            : TaskResource_(owner), mImage(image), mInfo(info) {
        }
        TaskImage_::~TaskImage_() {
            Owner()->ReleaseImageResource(this);
            Device()->DestroyDeferred(mImage);
            if (srvId != PYRO_NULL_SRV) {
                Device()->DestroyDeferred(srvId);
            }
            if (uavId != PYRO_NULL_UAV) {
                Device()->DestroyDeferred(uavId);
            }
        }
        ShaderResourceId TaskImage_::ShaderResource() const {
            std::lock_guard l(mShaderResourceLock);
            if (srvId == PYRO_NULL_SRV) {
                srvId = const_cast<TaskImage_*>(this)->Device()->CreateShaderResource(GetDefaultResourceInfo());
            }
            return srvId;
        }
        UnorderedAccessId TaskImage_::UnorderedAccess() const {
            std::lock_guard l(mShaderResourceLock);
            if (uavId == PYRO_NULL_UAV) {
                uavId = const_cast<TaskImage_*>(this)->Device()->CreateUnorderedAccess(GetDefaultResourceInfo());
            }
            return uavId;
        }
        ImageResourceInfo TaskImage_::GetDefaultResourceInfo() const {
            ImageResourceInfo info;
            info.image = mImage;
            info.slice.layerCount = mInfo.arrayLayerCount;
            info.slice.levelCount = mInfo.mipLevelCount;
            switch (mInfo.dimensions) {
            case ImageDimensions::e1D:
                info.viewType = mInfo.arrayLayerCount > 1 ? ImageViewType::e1DArray : ImageViewType::e1D;
                break;
            case ImageDimensions::e2D:
                if (mInfo.flags & ImageCreateFlagBits::CUBE) {
                    info.viewType = mInfo.arrayLayerCount > 6 ? ImageViewType::eCubeArray : ImageViewType::eCube;
                } else {
                    info.viewType = mInfo.arrayLayerCount > 1 ? ImageViewType::e2DArray : ImageViewType::e2D;
                }
                break;
            case ImageDimensions::e3D:
                info.viewType = ImageViewType::e3D;
                break;
            }
            return info;
        }

        TaskColorTarget_::TaskColorTarget_(TaskResourceManager* owner, const TaskColorTargetInfo& info, RenderTarget&& renderTarget)
            : TaskResource_(owner), mRenderTarget(renderTarget), mInfo(info) {
        }
        TaskColorTarget_::~TaskColorTarget_() {
            Device()->DestroyDeferred(mRenderTarget);
        }
        TaskDepthStencilTarget_::TaskDepthStencilTarget_(TaskResourceManager* owner, const TaskDepthStencilTargetInfo& info, RenderTarget&& renderTarget)
            : TaskResource_(owner), mRenderTarget(renderTarget), mInfo(info) {
        }
        TaskDepthStencilTarget_::~TaskDepthStencilTarget_() {
            Device()->DestroyDeferred(mRenderTarget);
        }
        TaskSwapChain_::TaskSwapChain_(TaskResourceManager* owner, const TaskSwapChainInfo& info, ISwapChain*&& swapChain)
            : TaskResource_(owner), mSwapChain(swapChain), mInfo(info) {
        }
        TaskSwapChain_::~TaskSwapChain_() {
            Device()->DestroyDeferred(mSwapChain);
        }
        TaskBlas_::TaskBlas_(TaskResourceManager* owner, const TaskBlasInfo& info, BlasId&& blas)
            : TaskResource_(owner), mBlas(blas), mInfo(info) {
        }
        TaskBlas_::~TaskBlas_() {
            Device()->DestroyDeferred(mBlas);
        }
        BlasAddress TaskBlas_::InstanceAddress() {
            return Device()->BlasInstanceAddress(mBlas);
        }
        TaskTlas_::TaskTlas_(TaskResourceManager* owner, const TaskTlasInfo& info, TlasId&& tlas)
            : TaskResource_(owner), mTlas(tlas), mInfo(info) {
        }
        TaskTlas_::~TaskTlas_() {
            Device()->DestroyDeferred(mTlas);
        }
    } // namespace Renderer
} // namespace PyroshockStudios