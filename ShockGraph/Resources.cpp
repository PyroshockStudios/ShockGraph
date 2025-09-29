#pragma once

#include "Resources.hpp"
#include "TaskResourceManager.hpp"
#include <PyroCommon/Core.hpp>
#include <PyroCommon/Memory.hpp>
#include <PyroRHI/Api/Forward.hpp>
#include <PyroRHI/Api/IDevice.hpp>
#include <PyroRHI/Api/GPUResource.hpp>
#include <PyroRHI/Api/Types.hpp>

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
        TaskComputePipeline_::TaskComputePipeline_(TaskResourceManager* owner, const TaskComputePipelineInfo& info, const TaskShaderInfo& shader)
            : TaskResource_(owner), mInfo(info), mShader(shader) {
        }
        TaskComputePipeline_::~TaskComputePipeline_() {
            mShader.program->RemoveReference(this);
            Device()->Destroy(mPipeline);
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
            Device()->Destroy(mBuffer);
            for (auto& buffer : mInFlightBuffers) {
                Device()->Destroy(buffer);
            }
        }
        u8* TaskBuffer_::MappedMemory() {
            // FIXME host visible buffer with no in flight buffers
            return Device()->BufferHostAddress(InternalInFlightBuffer(mCurrentBufferInFlight));
        }
        TaskImage_::TaskImage_(TaskResourceManager* owner, const TaskImageInfo& info, Image&& image)
            : TaskResource_(owner), mImage(image), mInfo(info) {
        }
        TaskImage_::~TaskImage_() {
            Owner()->ReleaseImageResource(this);
            Device()->Destroy(mImage);
        }
        TaskColorTarget_::TaskColorTarget_(TaskResourceManager* owner, const TaskColorTargetInfo& info, RenderTarget&& renderTarget)
            : TaskResource_(owner), mRenderTarget(renderTarget), mInfo(info) {
        }
        TaskColorTarget_::~TaskColorTarget_() {
            Device()->Destroy(mRenderTarget);
        }
        TaskDepthStencilTarget_::TaskDepthStencilTarget_(TaskResourceManager* owner, const TaskDepthStencilTargetInfo& info, RenderTarget&& renderTarget)
            : TaskResource_(owner), mRenderTarget(renderTarget), mInfo(info) {
        }
        TaskDepthStencilTarget_::~TaskDepthStencilTarget_() {
            Device()->Destroy(mRenderTarget);
        }
        TaskSwapChain_::TaskSwapChain_(TaskResourceManager* owner, const TaskSwapChainInfo& info, ISwapChain*&& swapChain)
            : TaskResource_(owner), mSwapChain(swapChain), mInfo(info) {
        }
        TaskSwapChain_::~TaskSwapChain_() {
            Device()->Destroy(mSwapChain);
        }
    }
}