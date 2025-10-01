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

#include "Task.hpp"
#include "TaskCommandList.hpp"
#include "TaskResourceManager.hpp"
#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>
#include <PyroCommon/LoggerInterface.hpp>
#include <PyroRHI/Api/Semaphore.hpp>

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
        class TaskGraph : public ILoggerAware, DeleteCopy, DeleteMove {
        public:
            SHOCKGRAPH_API TaskGraph(const TaskGraphInfo& info);
            SHOCKGRAPH_API ~TaskGraph();

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

            PYRO_NODISCARD SHOCKGRAPH_API eastl::string ToString();

            SHOCKGRAPH_API void InjectLogger(const ILogStream* stream) override {
                mLogStream = stream;
            }

            SHOCKGRAPH_API eastl::span<GenericTask*> GetTasks();

            /**
             * @brief Returns the GPU timings in nanoseconds of a specific task.
             */
            SHOCKGRAPH_API f64 GetTaskTimingsNs(GenericTask* task);
            /**
             * @brief Returns the GPU timings in nanoseconds of the entire task graph.
             */
            SHOCKGRAPH_API f64 GetGraphTimingsNs();
            /**
            * @brief Returns the GPU timings in nanoseconds of the per-frame flushes
            * such as staging buffers, dynamic buffers. This includes both buffer copies
            * and buffer/image barriers.
            */
            SHOCKGRAPH_API f64 GetMiscFlushesTimingsNs();

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

            eastl::vector<GenericTask*> mAllTaskRefs = {};
            u32 mBaseGraphTimestampIndex = 0;
            u32 mBaseMiscFlushesTimestampIndex = 0;

            IFence* mGpuFrameTimeline;
            eastl::vector<Semaphore> mRenderFinishedSemaphores;
            eastl::vector<ITimestampQueryPool*> mTimestampQueryPools;

            

            u32 mFrameIndex = 0;
            u32 mFramesInFlight = 0;
            u64 mCpuTimelineIndex = 0;
            bool bInFrame = false;
            bool bBaked = false;

            const ILogStream* mLogStream = nullptr;
        };
    } // namespace Renderer
} // namespace PyroshockStudios