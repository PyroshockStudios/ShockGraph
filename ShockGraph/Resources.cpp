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

namespace PyroshockStudios {
    inline namespace Renderer {
        SHOCKGRAPH_API TaskResource_ ::TaskResource_(TaskResourceManager* owner) : mOwner(owner) {
            mOwner->RegisterResource(this);
        }
        SHOCKGRAPH_API TaskResource_ ::~TaskResource_() {
            mOwner->ReleaseResource(this);
        }

        IDevice* TaskResource_::Device() {
            return mOwner->mDevice;
        }
        SHOCKGRAPH_API TaskShader_::TaskShader_(ShaderProgram&& program, FunctionPtr<void(void*, TaskShader_*)> deleter, void* deleterUserData)
            : mProgram(eastl::move(program)), mDeleter(eastl::move(deleter)), mDeleterUserData(deleterUserData) {}

        SHOCKGRAPH_API TaskShader_::~TaskShader_() { mDeleter(mDeleterUserData, this); }

        SHOCKGRAPH_API TaskRasterPipeline_::TaskRasterPipeline_(TaskResourceManager* owner, const TaskRasterPipelineInfo& info, const TaskRasterPipelineShaders& shaderStages)
            : TaskResource_(owner), mInfo(info), mStages(shaderStages) {
        }
        SHOCKGRAPH_API TaskRasterPipeline_::~TaskRasterPipeline_() {
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
            Device()->Destroy(mPipeline);
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
        SHOCKGRAPH_API TaskComputePipeline_::TaskComputePipeline_(TaskResourceManager* owner, const TaskComputePipelineInfo& info, const TaskShaderInfo& shader)
            : TaskResource_(owner), mInfo(info), mShader(shader) {
        }
        SHOCKGRAPH_API TaskComputePipeline_::~TaskComputePipeline_() {
            mShader.program->RemoveReference(this);
            Device()->Destroy(mPipeline);
        }
         void TaskComputePipeline_::Recreate() {
            ShaderInfo copyShader;
            copyShader.program = mShader.program->Program().bytecode;
            copyShader.specializationConstants = mShader.specializationConstants;
            mPipeline = Device()->Create(mInfo, copyShader);
        }

        SHOCKGRAPH_API TaskBuffer_::TaskBuffer_(TaskResourceManager* owner, const TaskBufferInfo& info, Buffer&& buffer, eastl::vector<Buffer>&& inFlightBuffers)
            : TaskResource_(owner), mBuffer(buffer), mInFlightBuffers(eastl::move(inFlightBuffers)), mInfo(info) {
        }
        SHOCKGRAPH_API TaskBuffer_::~TaskBuffer_() {
            Owner()->ReleaseBufferResource(this);
            Device()->Destroy(mBuffer);
            for (auto& buffer : mInFlightBuffers) {
                Device()->Destroy(buffer);
            }
        }
        SHOCKGRAPH_API u8* TaskBuffer_::MappedMemory() {
            // FIXME host visible buffer with no in flight buffers
            return Device()->BufferHostAddress(InternalInFlightBuffer(mCurrentBufferInFlight));
        }
        SHOCKGRAPH_API TaskImage_::TaskImage_(TaskResourceManager* owner, const TaskImageInfo& info, Image&& image)
            : TaskResource_(owner), mImage(image), mInfo(info) {
        }
        SHOCKGRAPH_API TaskImage_::~TaskImage_() {
            Owner()->ReleaseImageResource(this);
            Device()->Destroy(mImage);
        }
        SHOCKGRAPH_API TaskColorTarget_::TaskColorTarget_(TaskResourceManager* owner, const TaskColorTargetInfo& info, RenderTarget&& renderTarget)
            : TaskResource_(owner), mRenderTarget(renderTarget), mInfo(info) {
        }
        SHOCKGRAPH_API TaskColorTarget_::~TaskColorTarget_() {
            Device()->Destroy(mRenderTarget);
        }
        SHOCKGRAPH_API TaskDepthStencilTarget_::TaskDepthStencilTarget_(TaskResourceManager* owner, const TaskDepthStencilTargetInfo& info, RenderTarget&& renderTarget)
            : TaskResource_(owner), mRenderTarget(renderTarget), mInfo(info) {
        }
        SHOCKGRAPH_API TaskDepthStencilTarget_::~TaskDepthStencilTarget_() {
            Device()->Destroy(mRenderTarget);
        }
        SHOCKGRAPH_API TaskSwapChain_::TaskSwapChain_(TaskResourceManager* owner, const TaskSwapChainInfo& info, ISwapChain*&& swapChain)
            : TaskResource_(owner), mSwapChain(swapChain), mInfo(info) {
        }
        SHOCKGRAPH_API TaskSwapChain_::~TaskSwapChain_() {
            Device()->Destroy(mSwapChain);
        }
        SHOCKGRAPH_API TaskBlas_::TaskBlas_(TaskResourceManager* owner, const TaskBlasInfo& info, BlasId&& blas)
            : TaskResource_(owner), mBlas(blas), mInfo(info) {
        }
        SHOCKGRAPH_API TaskBlas_::~TaskBlas_() {
            Device()->Destroy(mBlas);
        }
        SHOCKGRAPH_API TaskTlas_::TaskTlas_(TaskResourceManager* owner, const TaskTlasInfo& info, TlasId&& tlas)
            : TaskResource_(owner), mTlas(tlas), mInfo(info) {
        }
        SHOCKGRAPH_API TaskTlas_::~TaskTlas_() {
            Device()->Destroy(mTlas);
        }
    } // namespace Renderer
} // namespace PyroshockStudios