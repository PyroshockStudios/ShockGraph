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

#pragma once
#include "Resources.hpp"

#include <PyroCommon/LoggerInterface.hpp>
#include <PyroRHI/Api/Forward.hpp>
#include <ShockGraph/Core.hpp>

namespace PyroshockStudios {
    inline namespace Renderer {
        class TaskGraph;
        struct TaskResource_;

        struct IShaderReloadListener;

        struct TaskResourceManagerInfo {
            RHIContext* rhi = nullptr;
            IDevice* device = nullptr;
            u32 framesInFlight = {};
        };
        struct TaskBufferResourceInfo {
            TaskBuffer buffer = {};
            BufferRegion region = {};
        };
        struct TaskImageResourceInfo {
            TaskImage image = {};
            ImageMipArraySlice slice = {};
            ImageViewType viewType = ImageViewType::e2D;
            Format format = Format::Inherit;
        };

        using TaskSamplerInfo = SamplerInfo;
        class TaskResourceManager : public ILoggerAware, DeleteCopy, DeleteMove {
        public:
            SHOCKGRAPH_API TaskResourceManager(const TaskResourceManagerInfo& info);
            SHOCKGRAPH_API ~TaskResourceManager();

            PYRO_NODISCARD SHOCKGRAPH_API TaskBuffer CreatePersistentBuffer(const TaskBufferInfo& info, eastl::span<const u8> initialData = {});
            PYRO_NODISCARD SHOCKGRAPH_API TaskImage CreatePersistentImage(const TaskImageInfo& info, eastl::span<const u8> initialData = {});

            PYRO_NODISCARD SHOCKGRAPH_API ShaderResourceId DefaultShaderResourceView(TaskImage image);
            PYRO_NODISCARD SHOCKGRAPH_API ShaderResourceId DefaultShaderResourceView(TaskBuffer image);

            PYRO_NODISCARD SHOCKGRAPH_API TaskColorTarget CreateColorTarget(const TaskColorTargetInfo& info);
            PYRO_NODISCARD SHOCKGRAPH_API TaskDepthStencilTarget CreateDepthStencilTarget(const TaskDepthStencilTargetInfo& info);

            PYRO_NODISCARD SHOCKGRAPH_API ShaderResourceId CreateShaderResourceView(const TaskBufferResourceInfo& info);
            PYRO_NODISCARD SHOCKGRAPH_API ShaderResourceId CreateShaderResourceView(const TaskImageResourceInfo& info);
            PYRO_NODISCARD SHOCKGRAPH_API UnorderedAccessId CreateUnorderedAccessView(const TaskBufferResourceInfo& info);
            PYRO_NODISCARD SHOCKGRAPH_API UnorderedAccessId CreateUnorderedAccessView(const TaskImageResourceInfo& info);
            PYRO_NODISCARD SHOCKGRAPH_API SamplerId CreateSampler(const TaskSamplerInfo& info);

            SHOCKGRAPH_API void ReleaseShaderResourceView(ShaderResourceId& id);
            SHOCKGRAPH_API void ReleaseUnorderedAccessView(UnorderedAccessId& id);
            SHOCKGRAPH_API void ReleaseSampler(SamplerId& id);

            PYRO_NODISCARD SHOCKGRAPH_API TaskRasterPipeline CreateRasterPipeline(const TaskRasterPipelineInfo& info, const TaskRasterPipelineShaders& shaders);
            PYRO_NODISCARD SHOCKGRAPH_API TaskComputePipeline CreateComputePipeline(const TaskComputePipelineInfo& info, const TaskShaderInfo& shader);

            PYRO_NODISCARD SHOCKGRAPH_API TaskSwapChain CreateSwapChain(const TaskSwapChainInfo& info);

            SHOCKGRAPH_API void SetFramesInFlight(u32 newFramesInFlight);

            PYRO_NODISCARD SHOCKGRAPH_API IShaderReloadListener* GetShaderReloadListener();

            PYRO_NODISCARD PYRO_FORCEINLINE IDevice* GetInternalDevice() {
                return mDevice;
            }
            PYRO_NODISCARD PYRO_FORCEINLINE RHIContext* GetInternalContext() {
                return mRHI;
            }

            SHOCKGRAPH_API void InjectLogger(const ILogStream* stream) override {
                mLogStream = stream;
            }

        private:
            void RegisterResource(TaskResource_* resource);
            void ReleaseResource(TaskResource_* resource);
            void ReleaseBufferResource(TaskBuffer_* resource);
            void ReleaseImageResource(TaskImage_* resource);
            friend struct TaskBuffer_;
            friend struct TaskImage_;

            eastl::vector<u32> mTombstones = {};
            eastl::vector<TaskResource_*> mResources = {};

            struct StagingUploadData {
                Buffer dstBuffer = {};
                BufferLayout dstBufferLayout = {};
                Image dstImage = {};
                ImageLayout dstImageLayout = {};
                ImageArraySlice dstImageSlice = {};
                u32 rowPitch = {};
            };
            struct StagingUploadPair {
                Buffer srcBuffer = {};
                eastl::vector<StagingUploadData> uploads = {};
            };
            eastl::vector<StagingUploadPair> mPendingStagingUploads = {};
            eastl::vector<TaskBuffer_*> mDynamicBuffers = {};

            IDevice* mDevice = nullptr;
            RHIContext* mRHI = nullptr;
            u32 mFramesInFlight = {};

            IShaderReloadListener* mShaderReloadListener = nullptr;
            const ILogStream* mLogStream = nullptr;

            friend class TaskGraph;
            friend struct TaskResource_;
        };
    } // namespace Renderer
} // namespace PyroshockStudios