#pragma once

#include "Task.hpp"
#include "TaskCommandList.hpp"
#include "TaskResourceManager.hpp"
#include <PyroRHI/Api/Semaphore.hpp>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>

namespace PyroshockStudios {
    inline namespace Renderer {
        using TaskId = u32;
        struct TaskGraphInfo {
            TaskResourceManager* resourceManager = nullptr;
        };
        class TaskExecute;

        struct TaskSwapChainWriteInfo {
            TaskImage image = nullptr;
            TaskSwapChain swapChain = nullptr;
            Rect2D srcRect = {};
            Rect2D dstRect = {};
        };
        class TaskGraph : DeleteCopy, DeleteMove {
        public:
            TaskGraph(const TaskGraphInfo& info);
            ~TaskGraph();

            SHOCKGRAPH_API void AddTask(GraphicsTask* task);
            SHOCKGRAPH_API void AddTask(ComputeTask* task);
            SHOCKGRAPH_API void AddTask(TransferTask* task);
            SHOCKGRAPH_API void AddTask(CustomTask* task);

            SHOCKGRAPH_API void AddSwapChainWrite(const TaskSwapChainWriteInfo& writeInfo);

            SHOCKGRAPH_API void Reset();
            SHOCKGRAPH_API void Build();

            SHOCKGRAPH_API void BeginFrame(u32 timeoutMilliseconds = 1000);
            SHOCKGRAPH_API void EndFrame();
            SHOCKGRAPH_API void Execute();

            eastl::string ToString();

        private:
            void FlushStagingBuffers(ICommandBuffer* commandBuffer);
            void FlushDynamicBuffers(ICommandBuffer* commandBuffer);

            IDevice* mDevice = {};
            TaskResourceManager* mResourceManager = {};

            struct Batch {
                eastl::vector<TaskId> taskIds = {};
                eastl::vector<BufferMemoryBarrierInfo> bufferBarriers = {};
                eastl::vector<ImageMemoryBarrierInfo> imageBarriers = {};
            };

            ICommandQueue* mQueue = nullptr;

            eastl::vector<eastl::unique_ptr<GenericTask>> mInternalTasks = {};
            eastl::vector<Batch> mBatches = {};
            eastl::vector<TaskExecute*> mTasks = {};
            eastl::vector<TaskSwapChain> mSwapChains = {};

            IFence* mGpuFrameTimeline;
            eastl::vector<Semaphore> mRenderFinishedSemaphores;

            u32 mFrameIndex = 0;
            u32 mFramesInFlight = 0;
            u64 mCpuTimelineIndex = 0;
            bool bInFrame = false;
            bool bBaked = false;
        };
    } // namespace Renderer
} // namespace PyroshockStudios