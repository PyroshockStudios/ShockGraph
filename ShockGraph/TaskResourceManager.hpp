#pragma once
#include "Resources.hpp"

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
        class TaskResourceManager : DeleteCopy, DeleteMove {
        public:
            TaskResourceManager(const TaskResourceManagerInfo& info);
            ~TaskResourceManager();

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

            friend class TaskGraph;
            friend struct TaskResource_;
        };
    } // namespace Renderer
} // namespace PyroshockStudios