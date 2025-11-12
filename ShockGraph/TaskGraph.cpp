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

#include "TaskGraph.hpp"
#include <PyroCommon/Logger.hpp>
#include <PyroRHI/Api/ICommandQueue.hpp>
#include <PyroRHI/Api/IDevice.hpp>

#include <EASTL/algorithm.h>
#include <EASTL/numeric.h>
#include <EASTL/sort.h>
#include <libassert/assert.hpp>

namespace PyroshockStudios {
    inline namespace Renderer {

        static BufferLayout AccessToBufferLayout(Access access) {
            bool bTransfer = false;
            bool bCompute = false;
            bool bGraphics = false;

            bool bRead = false;
            bool bWrite = false;

            if (access == 0) {
                return BufferLayout::Undefined;
            }

            if (access.stages & PipelineStageFlagBits::TRANSFER || access.stages & PipelineStageFlagBits::RESOLVE ||
                access.stages & PipelineStageFlagBits::BLIT || access.stages & PipelineStageFlagBits::COPY) {
                bTransfer = true;
            }
            if (access.stages & PipelineStageFlagBits::ALL_GRAPHICS) {
                bGraphics = true;
            }
            if (access.stages & PipelineStageFlagBits::COMPUTE_SHADER) {
                bCompute = true;
            }
            if (access.type & AccessTypeFlagBits::READ) {
                bRead = true;
            }
            if (access.type & AccessTypeFlagBits::WRITE) {
                bWrite = true;
            }

            if (bTransfer) {
                if (bRead && !bWrite) {
                    return BufferLayout::TransferSrc;
                }
                if (bWrite && !bRead) {
                    return BufferLayout::TransferDst;
                }
                ASSERT(false, "BAD BUFFER LAYOUT! Invalid combination of Transfer parameters chosen!");
            }
            if (bRead && !bWrite) {
                return BufferLayout::ReadOnly;
            }
            if (bWrite) {
                return BufferLayout::UnorderedAccess;
            }
            ASSERT(false, "BAD BUFFER LAYOUT! Invalid combination of parameters chosen!");
            return BufferLayout::Identity;
        }
        static ImageLayout AccessToImageLayout(Access access) {
            bool bBlit = false;
            bool bTransfer = false;
            bool bCompute = false;
            bool bGraphics = false;

            bool bRenderTarget = false;

            bool bRead = false;
            bool bWrite = false;
            if (access == 0) {
                return ImageLayout::Undefined;
            }


            if (access.stages & PipelineStageFlagBits::TRANSFER ||
                access.stages & PipelineStageFlagBits::COPY) {
                bTransfer = true;
            }
            if (access.stages & PipelineStageFlagBits::BLIT) {
                bBlit = true;
            }
            if (access.stages & PipelineStageFlagBits::ALL_GRAPHICS) {
                bGraphics = true;
            }
            if (access.stages & PipelineStageFlagBits::COMPUTE_SHADER) {
                bCompute = true;
            }
            if (access.stages & PipelineStageFlagBits::COLOR_ATTACHMENT_OUTPUT ||
                access.stages & PipelineStageFlagBits::EARLY_FRAGMENT_TESTS || access.stages & PipelineStageFlagBits::LATE_FRAGMENT_TESTS) {
                bRenderTarget = true;
            }
            if (access.type & AccessTypeFlagBits::READ) {
                bRead = true;
            }
            if (access.type & AccessTypeFlagBits::WRITE) {
                bWrite = true;
            }

            if (bTransfer) {
                if (bRead && !bWrite) {
                    return ImageLayout::TransferSrc;
                }
                if (bWrite && !bRead) {
                    return ImageLayout::TransferDst;
                }
                ASSERT(false, "BAD IMAGE LAYOUT! Invalid combination of parameters chosen!");
            }
            if (bBlit) {
                if (bRead && !bWrite) {
                    return ImageLayout::BlitSrc;
                }
                if (bWrite && !bRead) {
                    return ImageLayout::BlitDst;
                }
                ASSERT(false, "BAD IMAGE LAYOUT! Invalid combination of parameters chosen!");
            }

            if (bRenderTarget) {
                if (bWrite) {
                    return ImageLayout::RenderTarget;
                }
                if (bRead) {
                    return ImageLayout::RenderTargetReadOnly;
                }
                ASSERT(false, "BAD IMAGE LAYOUT! Invalid combination of parameters chosen!");
            }
            if (bRead && !bWrite) {
                return ImageLayout::ReadOnly;
            }
            if (bWrite) {
                return ImageLayout::UnorderedAccess;
            }
            ASSERT(false, "BAD IMAGE LAYOUT! Invalid combination of parameters chosen!");
            return ImageLayout::Identity;
        }


        class TaskExecute : DeleteCopy, DeleteMove {
        public:
            TaskExecute(GenericTask* task) : mTask(task) {
            }
            virtual ~TaskExecute() = default;

            virtual void PreExec(ICommandBuffer* commandBuffer) {
                commandBuffer->BeginLabel({
                    .labelColor = mTask->Info().color,
                    .name = mTask->Info().name,
                });
                commandBuffer->WriteTimestamp({
                    .queryPool = mTimestampPool,
                    .stage = PipelineStageFlagBits::TOP_OF_PIPE,
                    .queryIndex = mBaseTimestampIndex,
                });
            }
            virtual void PostExec(ICommandBuffer* commandBuffer) {
                commandBuffer->WriteTimestamp({
                    .queryPool = mTimestampPool,
                    .stage = PipelineStageFlagBits::BOTTOM_OF_PIPE,
                    .queryIndex = mBaseTimestampIndex + 1,
                });
                commandBuffer->EndLabel();
            }

            PYRO_NODISCARD PYRO_FORCEINLINE GenericTask* GetTask() {
                return mTask;
            }

            ITimestampQueryPool* mTimestampPool = nullptr;
            u32 mBaseTimestampIndex = 0;

        private:
            GenericTask* mTask = {};
        };
        class GraphicsTaskExecute : public TaskExecute {
        public:
            GraphicsTaskExecute(GraphicsTask* task, RenderPassBeginInfo&& renderPassBeginInfo)
                : TaskExecute(task), mRenderPassInfo(eastl::move(renderPassBeginInfo)) {
            }
            ~GraphicsTaskExecute() = default;
            void PreExec(ICommandBuffer* commandBuffer) override {
                TaskExecute::PreExec(commandBuffer);
                commandBuffer->BeginRenderPass(mRenderPassInfo);
            }
            void PostExec(ICommandBuffer* commandBuffer) override {
                commandBuffer->EndRenderPass();
                TaskExecute::PostExec(commandBuffer);
            }

        private:
            RenderPassBeginInfo mRenderPassInfo = {};
        };
        class ComputeTaskExecute : public TaskExecute {
        public:
            ComputeTaskExecute(ComputeTask* task) : TaskExecute(task) {}
            ~ComputeTaskExecute() = default;
        };
        class TransferTaskExecute : public TaskExecute {
        public:
            TransferTaskExecute(TransferTask* task) : TaskExecute(task) {}
            ~TransferTaskExecute() = default;
        };

        SHOCKGRAPH_API TaskGraph::TaskGraph(const TaskGraphInfo& info)
            : mDevice(info.resourceManager->mDevice), mQueue(mDevice->GetPresentQueue()), mResourceManager(info.resourceManager),
              mFramesInFlight(info.resourceManager->mFramesInFlight) {

            mGpuFrameTimeline = mDevice->CreateFence({ .name = "Task Graph GPU Timeline" });
            for (usize i = 0; i < mFramesInFlight; ++i) {
                mRenderFinishedSemaphores.push_back(mDevice->CreateSemaphore({ "Task Graph Render Finish Semaphore #" + eastl::to_string(i) }));
            }
        }
        SHOCKGRAPH_API TaskGraph::~TaskGraph() {
            mDevice->WaitIdle();
            // cleanup resources
            this->Reset();
            mDevice->Destroy(mGpuFrameTimeline);
            for (auto& sem : mRenderFinishedSemaphores) {
                mDevice->Destroy(sem);
            }
        }

        SHOCKGRAPH_API void TaskGraph::AddTask(GraphicsTask* task) {
            ASSERT(task);
            ASSERT(!bBaked, "Cannot add to a task graph after it was built!");
            task->SetupTask();
            RenderPassBeginInfo renderPassInfo{};
            renderPassInfo.colorAttachments.reserve(task->mGraphicsSetupData.colorTargets.size());
            for (const auto& colorTarget : task->mGraphicsSetupData.colorTargets) {
                ColorAttachmentInfo attachmentInfo = {};
                attachmentInfo.target = colorTarget.target->Internal();
                if (colorTarget.clear) {
                    attachmentInfo.clearValue = *colorTarget.clear;
                    attachmentInfo.loadOp = AttachmentLoadOp::Clear;
                } else {
                    attachmentInfo.loadOp = AttachmentLoadOp::Load;
                }
                attachmentInfo.storeOp = AttachmentStoreOp::Store;
                if (colorTarget.resolve) {
                    attachmentInfo.resolve.emplace(ResolveMode::Average,
                        colorTarget.resolve.value()->Internal());
                }
                renderPassInfo.colorAttachments.emplace_back(eastl::move(attachmentInfo));
            }
            if (task->mGraphicsSetupData.depthStencilTarget) {
                const auto& depthStencil = *task->mGraphicsSetupData.depthStencilTarget;
                DepthStencilAttachmentInfo attachmentInfo = {};
                attachmentInfo.target = depthStencil.target->Internal();
                if (depthStencil.depthClear) {
                    attachmentInfo.clearValue.depth = *depthStencil.depthClear;
                    attachmentInfo.depthLoadOp = AttachmentLoadOp::Clear;
                } else {
                    attachmentInfo.depthLoadOp = depthStencil.bDepth ? AttachmentLoadOp::Load : AttachmentLoadOp::DontCare;
                }
                if (depthStencil.stencilClear) {
                    attachmentInfo.clearValue.stencil = *depthStencil.stencilClear;
                    attachmentInfo.stencilLoadOp = AttachmentLoadOp::Clear;
                } else {
                    attachmentInfo.stencilLoadOp = depthStencil.bStencil ? AttachmentLoadOp::Load : AttachmentLoadOp::DontCare;
                }
                if (depthStencil.bDepthStore) {
                    attachmentInfo.depthStoreOp = AttachmentStoreOp::Store;
                } else {
                    attachmentInfo.depthStoreOp = AttachmentStoreOp::DontCare;
                }
                if (depthStencil.bStencilStore) {
                    attachmentInfo.stencilStoreOp = AttachmentStoreOp::Store;
                } else {
                    attachmentInfo.stencilStoreOp = AttachmentStoreOp::DontCare;
                }
                renderPassInfo.depthStencilAttachment.emplace(eastl::move(attachmentInfo));
            }
            // FIXME stencil buffer
            Extent3D extent;
            if (!renderPassInfo.colorAttachments.empty()) {
                extent = mDevice->GetImageInfo(mDevice->GetRenderTargetInfo(renderPassInfo.colorAttachments[0].target).image).size;
            } else if (renderPassInfo.depthStencilAttachment.has_value()) {
                extent = mDevice->GetImageInfo(mDevice->GetRenderTargetInfo(renderPassInfo.depthStencilAttachment->target).image).size;
            } else {
                ASSERT(false, "NO render targets defined!");
            }
            renderPassInfo.renderArea = {
                .width = static_cast<i32>(extent.width),
                .height = static_cast<i32>(extent.height),
            };
            mAllTaskRefs.push_back(task);
            mTasks.push_back(new GraphicsTaskExecute(task, eastl::move(renderPassInfo)));
        }
        SHOCKGRAPH_API void TaskGraph::AddTask(ComputeTask* task) {
            ASSERT(task);
            ASSERT(!bBaked, "Cannot add to a task graph after it was built!");
            task->SetupTask();
            mAllTaskRefs.push_back(task);
            mTasks.push_back(new ComputeTaskExecute(task));
        }
        SHOCKGRAPH_API void TaskGraph::AddTask(TransferTask* task) {
            ASSERT(task);
            ASSERT(!bBaked, "Cannot add to a task graph after it was built!");
            task->SetupTask();
            mAllTaskRefs.push_back(task);
            mTasks.push_back(new TransferTaskExecute(task));
        }
        SHOCKGRAPH_API void TaskGraph::AddTask(CustomTask* task) {
            ASSERT(task);
            ASSERT(!bBaked, "Cannot add to a task graph after it was built!");
            task->SetupTask();
            mAllTaskRefs.push_back(task);
            mTasks.push_back(new TaskExecute(task));
        }

        SHOCKGRAPH_API void TaskGraph::AddSwapChainWrite(const TaskSwapChainWriteInfo& writeInfo) {
            ASSERT(writeInfo.swapChain);
            ASSERT(writeInfo.image);
            ASSERT(!bBaked, "Cannot add to a task graph after it was built!");
            ASSERT(writeInfo.image->Info().usage & ImageUsageFlagBits::BLIT_SRC &&
                       writeInfo.image->Info().usage & ImageUsageFlagBits::TRANSFER_SRC,
                "Image must be created with BLIT_SRC and TRANSFER_SRC usages!");
            mSwapChains.push_back(writeInfo.swapChain);
            mInternalTasks.emplace_back(eastl::make_unique<CustomCallbackTask>(
                TaskInfo{
                    .name = "Write Swap Buffer",
                    .color = LabelColor::BLACK,
                },
                [this, writeInfo](CustomTask& task) {
                    task.UseImage({ .image = writeInfo.image, .access = AccessConsts::BLIT_READ });
                },
                [this, writeInfo](ICommandBuffer* commandBuffer) {
                    i32 result = writeInfo.swapChain->Internal()->AcquireNextImage();
                    if (result == PYRO_SWAPCHAIN_ACQUIRE_FAIL) {
                        return;
                    }
                    Image swapImage = writeInfo.swapChain->Internal()->GetBackBuffer(result);
                    commandBuffer->ImageBarrier({
                        .image = swapImage,
                        .srcAccess = AccessConsts::BOTTOM_OF_PIPE_READ,
                        .dstAccess = AccessConsts::BLIT_WRITE,
                        .srcLayout = ImageLayout::Undefined,
                        .dstLayout = ImageLayout::BlitDst,
                    });
                    commandBuffer->BlitImageToImage({
                        .srcImage = writeInfo.image->Internal(),
                        .dstImage = swapImage,
                        .srcImageBox = { 
                            .x = writeInfo.srcRect.x, 
                            .y = writeInfo.srcRect.y, 
                            .z = 0, 
                            .width = writeInfo.srcRect.width, 
                            .height = writeInfo.srcRect.height, 
                            .depth = 1 
                        },
                        .dstImageBox = { 
                            .x = writeInfo.dstRect.x, 
                            .y = writeInfo.dstRect.y, 
                            .z = 0, 
                            .width = writeInfo.dstRect.width, 
                            .height = writeInfo.dstRect.height, 
                            .depth = 1 
                        },
                    });
                    commandBuffer->ImageBarrier({
                        .image = swapImage,
                        .srcAccess = AccessConsts::BLIT_WRITE,
                        .dstAccess = AccessConsts::TOP_OF_PIPE_READ_WRITE,
                        .srcLayout = ImageLayout::BlitDst,
                        .dstLayout = ImageLayout::PresentSrc,
                    });
                },
                TaskType::Transfer));
            mInternalTasks.back()->SetupTask();
            mAllTaskRefs.push_back(mInternalTasks.back().get());
            mTasks.push_back(new TaskExecute(mInternalTasks.back().get()));
        }

        SHOCKGRAPH_API void TaskGraph::Reset() {
            for (TaskExecute* task : mTasks) {
                delete task;
            }
            if (!mTimestampQueryPools.empty()) {
                // FIXME: remove this wait idle once we have a command list DestroyDeferred function for this!
                mDevice->WaitIdle();
                for (i32 i = 0; i < mFramesInFlight; ++i) {
                    mDevice->Destroy(mTimestampQueryPools[i]);
                }
                mTimestampQueryPools.clear();
            }
            mSwapChains.clear();
            mInternalTasks.clear();
            mTasks.clear();
            mBatches.clear();
            mAllTaskRefs.clear();
            bBaked = false;
        }
        SHOCKGRAPH_API void TaskGraph::Build() {
            Logger::Trace(mLogStream, "Rebuilding tasks");

            struct ResourceState {
                TaskAccessType currentAccess = {};
                eastl::optional<TaskId> lastTaskId = {};
            };

            struct TaskState {
                eastl::vector<TaskId> parents = {};
            };

            eastl::vector<ResourceState> currentResources = {};
            currentResources.resize(mResourceManager->mResources.size());

            eastl::vector<TaskState> currentTasks = {};
            currentTasks.resize(mTasks.size());

            // finds which tasks depends on each other
            for (u32 taskIndex = 0; taskIndex < mTasks.size(); taskIndex++) {
                TaskExecute*& task = mTasks[taskIndex];
                for (const auto& bufferDep : task->GetTask()->mSetupData.bufferDepends) {
                    u32 dependencyIndex = bufferDep.buffer->GetId();
                    ResourceState& depedencyState = currentResources[dependencyIndex];
                    if (depedencyState.lastTaskId.has_value()) {
                        currentTasks[taskIndex].parents.push_back(depedencyState.lastTaskId.value());
                    }
                    depedencyState.lastTaskId = eastl::make_optional(taskIndex);
                }
                for (const auto& imageDep : task->GetTask()->mSetupData.imageDepends) {
                    u32 dependencyIndex = imageDep.image->GetId();
                    ResourceState& depedencyState = currentResources[dependencyIndex];
                    if (depedencyState.lastTaskId.has_value()) {
                        currentTasks[taskIndex].parents.push_back(depedencyState.lastTaskId.value());
                    }
                    depedencyState.lastTaskId = eastl::make_optional(taskIndex);
                }
            }

            eastl::vector<TaskId> taskQueue = {};
            taskQueue.resize(currentTasks.size());
            eastl::iota(taskQueue.begin(), taskQueue.end(), static_cast<TaskId>(0));

            // does topological sort / batching
            while (!taskQueue.empty()) {
                eastl::vector<TaskId> tasksWithoutParents = {};

                for (TaskId taskIndex : taskQueue) {
                    if (currentTasks[taskIndex].parents.empty()) {
                        tasksWithoutParents.push_back(taskIndex);
                    }
                }

                taskQueue.erase(eastl::remove_if(taskQueue.begin(), taskQueue.end(), [&](int x) {
                    return eastl::find(tasksWithoutParents.begin(), tasksWithoutParents.end(), x) != tasksWithoutParents.end();
                }),
                    taskQueue.end());

                for (TaskId taskIndex : taskQueue) {
                    TaskState& task = currentTasks[taskIndex];
                    if (task.parents.empty()) {
                        continue;
                    }

                    for (usize parent : tasksWithoutParents) {
                        auto it = eastl::remove_if(task.parents.begin(), task.parents.end(), [&](TaskId& a) {
                            return a == parent;
                        });
                        task.parents.erase(it, task.parents.end());
                    }
                }

                mBatches.push_back({});
                auto& batch = mBatches.back();
                batch.taskIds = eastl::move(tasksWithoutParents);
            }

            // trackes the state of resources between batches / adds barriers
            for (Batch& batch : mBatches) {
                for (TaskId taskIndex : batch.taskIds) {
                    TaskExecute*& task = mTasks[taskIndex];

                    for (const auto& bufferDep : task->GetTask()->mSetupData.bufferDepends) {
                        u32 dependencyIndex = bufferDep.buffer->GetId();
                        ResourceState& dependencyState = currentResources[dependencyIndex];
                        if (dependencyState.currentAccess != bufferDep.access) {
                            BufferMemoryBarrierInfo barrier{};
                            barrier.buffer = bufferDep.buffer->Internal();
                            barrier.srcLayout = AccessToBufferLayout(dependencyState.currentAccess);
                            barrier.srcAccess = dependencyState.currentAccess;
                            barrier.dstLayout = AccessToBufferLayout(bufferDep.access);
                            barrier.dstAccess = bufferDep.access;
                            batch.bufferBarriers.push_back(barrier);
                            dependencyState.currentAccess = bufferDep.access;
                        }
                    }
                    for (const auto& imageDep : task->GetTask()->mSetupData.imageDepends) {
                        u32 dependencyIndex = imageDep.image->GetId();
                        ResourceState& dependencyState = currentResources[dependencyIndex];
                        if (dependencyState.currentAccess != imageDep.access) {
                            ImageMemoryBarrierInfo barrier{};
                            barrier.image = imageDep.image->Internal();
                            barrier.srcLayout = AccessToImageLayout(dependencyState.currentAccess);
                            barrier.srcAccess = dependencyState.currentAccess;
                            barrier.dstLayout = AccessToImageLayout(imageDep.access);
                            barrier.dstAccess = imageDep.access;
                            batch.imageBarriers.push_back(barrier);
                            dependencyState.currentAccess = imageDep.access;
                        }
                    }
                }
            }

            TaskType previousTaskType = TaskType::None;
            for (size_t i = 0; i < mBatches.size(); ++i) {
                Batch& batch = mBatches[i];

                TaskType nextBatchFirstType = TaskType::None;
                if (i + 1 < mBatches.size()) {
                    for (TaskId id : mBatches[i + 1].taskIds) {
                        nextBatchFirstType = mTasks[id]->GetTask()->GetType();
                        break;
                    }
                }

                eastl::sort(batch.taskIds.begin(), batch.taskIds.end(), [this, previousTaskType, nextBatchFirstType](TaskId a, TaskId b) {
                    GenericTask* taskA = mTasks[a]->GetTask();
                    GenericTask* taskB = mTasks[b]->GetTask();

                    if (taskA->GetType() == previousTaskType && taskB->GetType() != previousTaskType) {
                        return true;
                    }

                    if (taskA->GetType() != previousTaskType && taskB->GetType() == previousTaskType) {
                        return false;
                    }

                    if (nextBatchFirstType != TaskType::None) {
                        if (taskA->GetType() == nextBatchFirstType && taskB->GetType() != nextBatchFirstType) {
                            return false;
                        }
                        if (taskA->GetType() != nextBatchFirstType && taskB->GetType() == nextBatchFirstType) {
                            return true;
                        }
                    }

                    return taskA->GetType() < taskB->GetType();
                });

                if (!batch.taskIds.empty()) {
                    previousTaskType = mTasks[batch.taskIds.back()]->GetTask()->GetType();
                }
            }
            Logger::Trace(mLogStream, "Injecting timestamp profilers");
            for (i32 i = 0; i < mFramesInFlight; ++i) {
                mTimestampQueryPools.push_back(mDevice->CreateTimestampQueryPool({
                    .queryCount = static_cast<u32>(mTasks.size() * 2 + 4),
                    .name = "Timestamp query pool FiF=" + eastl::to_string(i),
                }));
            }
            mBaseGraphTimestampIndex = mTasks.size() * 2;
            mBaseMiscFlushesTimestampIndex = mTasks.size() * 2 + 2;
            for (i32 i = 0; i < mTasks.size(); ++i) {
                mTasks[i]->mBaseTimestampIndex = 2 * i;
            }
            bBaked = true;
            Logger::Trace(mLogStream, "Rebuilt task graph, {} task objects, {} batch objects", mTasks.size(), mBatches.size());
        }
        SHOCKGRAPH_API void TaskGraph::BeginFrame(u32 timeoutMilliseconds) {
            ASSERT(bBaked, "Build() must be called before starting a frame in a rendergraph!");
            // i dont know why, but cpu timeline index has to be 1 frame ahead than normal...
            ++mCpuTimelineIndex;
            bInFrame = true;
            for (TaskSwapChain& swapChain : mSwapChains) {
                if (swapChain->bFlagResize) {
                    swapChain->bFlagResize = false;
                    mDevice->WaitIdle();
                    swapChain->Internal()->Resize();
                }
            }

            u64 waitIndex = static_cast<u64>(
                std::max<i64>(
                    0,
                    static_cast<i64>(mCpuTimelineIndex) - static_cast<i64>(mFramesInFlight)));
            if (!mGpuFrameTimeline->WaitForValue(waitIndex, 1000 * 1000 * timeoutMilliseconds)) {
                Logger::Fatal(mLogStream, "GPU hanging! Aborting program!");
            }
        }
        SHOCKGRAPH_API void TaskGraph::EndFrame() {
            for (TaskSwapChain& swapChain : mSwapChains) {
                mQueue->SubmitSwapChain(swapChain->Internal());
            }
            eastl::array signalTimeline = { FenceSubmitInfo{ mGpuFrameTimeline, mCpuTimelineIndex } };
            eastl::array signalBinary = { SemaphoreSubmitInfo{ mRenderFinishedSemaphores[mFrameIndex], PipelineStageFlagBits::ALL_COMMANDS } };
            eastl::array waitBinary = { mRenderFinishedSemaphores[mFrameIndex] };
            mDevice->SubmitQueue({ .queue = mQueue, .signalPresentReadySemaphores = signalBinary, .signalFences = signalTimeline });
            mDevice->PresentQueue({ .queue = mQueue, .waitSemaphores = waitBinary });
            mFrameIndex = (mFrameIndex + 1) % mFramesInFlight;
            bInFrame = false;
        }
        SHOCKGRAPH_API void TaskGraph::Execute() {
            ASSERT(bInFrame, "Do not call Execute() outside of a frame!");

            ICommandBuffer* commandBuffer = mQueue->GetCommandBuffer({
                .name = mQueue->Info().name + "'s Task Graph Commands, #" + eastl::to_string(mFrameIndex),
            });
            commandBuffer->InvalidateTimestampQuery({
                .queryPool = mTimestampQueryPools[mFrameIndex],
                .firstQuery = 0,
                .queryCount = mTimestampQueryPools[mFrameIndex]->Info().queryCount,
            });
            { // TASK GRAPH BEGIN
                commandBuffer->WriteTimestamp({
                    .queryPool = mTimestampQueryPools[mFrameIndex],
                    .stage = PipelineStageFlagBits::TOP_OF_PIPE,
                    .queryIndex = mBaseGraphTimestampIndex,
                });
                { // FLUSHES BEGIN
                    commandBuffer->WriteTimestamp({
                        .queryPool = mTimestampQueryPools[mFrameIndex],
                        .stage = PipelineStageFlagBits::TOP_OF_PIPE,
                        .queryIndex = mBaseMiscFlushesTimestampIndex,
                    });

                    FlushStagingBuffers(commandBuffer);
                    FlushDynamicBuffers(commandBuffer);

                    commandBuffer->WriteTimestamp({
                        .queryPool = mTimestampQueryPools[mFrameIndex],
                        .stage = PipelineStageFlagBits::BOTTOM_OF_PIPE,
                        .queryIndex = mBaseMiscFlushesTimestampIndex + 1,
                    });
                } // FLUSHES END
                TaskCommandList wrapper{};
                wrapper.mCommandBuffer = commandBuffer;
                u32 batchIndex = 0;
                for (Batch& batch : mBatches) {
                    commandBuffer->BeginLabel({ .labelColor = LabelColor::BLACK,
                        .name = "Sync Barriers Batch #" + eastl::to_string(batchIndex) });
                    for (const auto& barrier : batch.imageBarriers) {
                        commandBuffer->ImageBarrier(barrier);
                    }
                    for (const auto& barrier : batch.bufferBarriers) {
                        commandBuffer->BufferBarrier(barrier);
                    }
                    commandBuffer->EndLabel();
                    for (TaskId taskIndex : batch.taskIds) {
                        TaskExecute* task = mTasks[taskIndex];
                        task->mTimestampPool = mTimestampQueryPools[mFrameIndex];
                        wrapper.mCurrBindPoint = task->GetTask()->GetBindPoint();
                        task->PreExec(commandBuffer);
                        task->GetTask()->ExecuteTask(wrapper);
                        task->PostExec(commandBuffer);
                    }
                    ++batchIndex;
                }

                commandBuffer->WriteTimestamp({
                    .queryPool = mTimestampQueryPools[mFrameIndex],
                    .stage = PipelineStageFlagBits::BOTTOM_OF_PIPE,
                    .queryIndex = mBaseGraphTimestampIndex + 1,
                });
            } // TASK GRAPH END
            commandBuffer->Complete();
            mQueue->SubmitCommandBuffer(commandBuffer);
        }


        SHOCKGRAPH_API eastl::span<GenericTask*> TaskGraph::GetTasks() {
            return mAllTaskRefs;
        }

        SHOCKGRAPH_API f64 TaskGraph::GetTaskTimingsNs(GenericTask* task) {
            for (TaskExecute* taskExec : mTasks) {
                if (taskExec->GetTask() != task)
                    continue;
                ITimestampQueryPool* pool = mTimestampQueryPools[(mFrameIndex + 1) % mFramesInFlight];
                eastl::span timestamps = pool->GetTimestamps(taskExec->mBaseTimestampIndex, 2);
                return static_cast<f64>(timestamps[1] - timestamps[0]) * mQueue->GetTimestampTickPeriodNs();
            }
            return 0.0;
        }

        SHOCKGRAPH_API f64 TaskGraph::GetGraphTimingsNs() {
            ITimestampQueryPool* pool = mTimestampQueryPools[(mFrameIndex + 1) % mFramesInFlight];
            eastl::span timestamps = pool->GetTimestamps(mBaseGraphTimestampIndex, 2);
            return static_cast<f64>(timestamps[1] - timestamps[0]) * mQueue->GetTimestampTickPeriodNs();
        }

        SHOCKGRAPH_API f64 TaskGraph::GetMiscFlushesTimingsNs() {
            ITimestampQueryPool* pool = mTimestampQueryPools[(mFrameIndex + 1) % mFramesInFlight];
            eastl::span timestamps = pool->GetTimestamps(mBaseMiscFlushesTimestampIndex, 2);
            return static_cast<f64>(timestamps[1] - timestamps[0]) * mQueue->GetTimestampTickPeriodNs();
        }

        void TaskGraph::FlushStagingBuffers(ICommandBuffer* commandBuffer) {
            commandBuffer->BeginLabel({ .labelColor = LabelColor::BLUE,
                .name = "Flush staging buffers" });
            for (auto& uploadPair : mResourceManager->mPendingStagingUploads) {
                commandBuffer->BufferBarrier({
                    .buffer = uploadPair.srcBuffer,
                    .srcAccess = AccessConsts::HOST_WRITE,
                    .dstAccess = AccessConsts::TRANSFER_READ,
                    .srcLayout = BufferLayout::TransferSrc,
                    .dstLayout = BufferLayout::TransferSrc,
                });
                for (auto& stagingUpload : uploadPair.uploads) {
                    if (stagingUpload.dstBuffer) {
                        commandBuffer->BufferBarrier({
                            .buffer = stagingUpload.dstBuffer,
                            .srcAccess = AccessConsts::NONE,
                            .dstAccess = AccessConsts::TRANSFER_WRITE,
                            .srcLayout = BufferLayout::Undefined,
                            .dstLayout = BufferLayout::TransferDst,
                        });
                        commandBuffer->CopyBufferToBuffer({
                            .srcBuffer = uploadPair.srcBuffer,
                            .dstBuffer = stagingUpload.dstBuffer,
                            .size = mDevice->GetBufferInfo(stagingUpload.dstBuffer).size,
                        });
                        commandBuffer->BufferBarrier({
                            .buffer = stagingUpload.dstBuffer,
                            .srcAccess = AccessConsts::TRANSFER_WRITE,
                            .dstAccess = AccessConsts::READ_WRITE, // TODO, not very efficient
                            .srcLayout = BufferLayout::TransferDst,
                            .dstLayout = stagingUpload.dstBufferLayout,
                        });
                    }
                    if (stagingUpload.dstImage) {
                        commandBuffer->ImageBarrier({
                            .image = stagingUpload.dstImage,
                            .srcAccess = AccessConsts::NONE,
                            .dstAccess = AccessConsts::TRANSFER_WRITE,
                            .srcLayout = ImageLayout::Undefined,
                            .dstLayout = ImageLayout::TransferDst,
                        });
                        // FIXME: multiple slices?
                        commandBuffer->CopyBufferToImage({ .buffer = uploadPair.srcBuffer,
                            .image = stagingUpload.dstImage,
                            .imageSlice = stagingUpload.dstImageSlice,
                            .imageExtent = mDevice->GetImageInfo(stagingUpload.dstImage).size,
                            .rowPitch = stagingUpload.rowPitch });
                        commandBuffer->ImageBarrier({
                            .image = stagingUpload.dstImage,
                            .srcAccess = AccessConsts::TRANSFER_WRITE,
                            .dstAccess = AccessConsts::READ_WRITE, // TODO, not very efficient
                            .srcLayout = ImageLayout::TransferDst,
                            .dstLayout = stagingUpload.dstImageLayout,
                        });
                    }
                }
                mDevice->Destroy(uploadPair.srcBuffer, true);
            }
            mResourceManager->mPendingStagingUploads.clear();

            commandBuffer->EndLabel();
        }
        void TaskGraph::FlushDynamicBuffers(ICommandBuffer* commandBuffer) {
            commandBuffer->BeginLabel({ .labelColor = LabelColor::BLUE,
                .name = "Flush dynamic buffers" });
            for (const auto& bufferCopy : mResourceManager->mDynamicBuffers) {
                bufferCopy->mCurrentBufferInFlight = mFrameIndex;
                if (bufferCopy->Info().bCpuVisible) {
                    bufferCopy->mBuffer = bufferCopy->InternalInFlightBuffer(mFrameIndex);
                } else {
                    commandBuffer->BufferBarrier({
                        .buffer = bufferCopy->InternalInFlightBuffer(mFrameIndex),
                        .srcAccess = AccessConsts::HOST_WRITE,
                        .dstAccess = AccessConsts::TRANSFER_READ,
                        .srcLayout = BufferLayout::TransferSrc,
                        .dstLayout = BufferLayout::TransferSrc,
                    });
                    commandBuffer->BufferBarrier({
                        .buffer = bufferCopy->Internal(),
                        .srcAccess = AccessConsts::NONE,
                        .dstAccess = AccessConsts::TRANSFER_WRITE,
                        .srcLayout = BufferLayout::Undefined,
                        .dstLayout = BufferLayout::TransferDst,
                    });
                    commandBuffer->CopyBufferToBuffer({
                        .srcBuffer = bufferCopy->InternalInFlightBuffer(mFrameIndex),
                        .dstBuffer = bufferCopy->Internal(),
                        .size = bufferCopy->Info().size,
                    });
                    /* TODO, how do we move these barriers to be closer to the access?
                        Maybe we add it as an access for USING BUFFER?
                    */
                    commandBuffer->BufferBarrier({
                        .buffer = bufferCopy->Internal(),
                        .srcAccess = AccessConsts::TRANSFER_READ,
                        .dstAccess = AccessConsts::READ,
                        .srcLayout = BufferLayout::TransferDst,
                        .dstLayout = BufferLayout::ReadOnly,
                    });
                }
            }
            commandBuffer->EndLabel();
        }

        SHOCKGRAPH_API eastl::string TaskGraph::ToString() {
            eastl::string out;
            out += "TaskGraph Batches:\n";

            static const auto ImageLayoutToString = [](ImageLayout layout) -> eastl::string {
                switch (layout) {
                case ImageLayout::Identity:
                    return "Identity";
                case ImageLayout::Undefined:
                    return "Undefined";
                case ImageLayout::UnorderedAccess:
                    return "UnorderedAccess";
                case ImageLayout::ReadOnly:
                    return "ReadOnly";
                case ImageLayout::RenderTarget:
                    return "RenderTarget";
                case ImageLayout::TransferSrc:
                    return "TransferSrc";
                case ImageLayout::TransferDst:
                    return "TransferDst";
                case ImageLayout::BlitSrc:
                    return "BlitSrc";
                case ImageLayout::BlitDst:
                    return "BlitDst";
                case ImageLayout::PresentSrc:
                    return "PresentSrc";
                default:
                    return "Unknown";
                }
            };

            static const auto BufferLayoutToString = [](BufferLayout layout) -> eastl::string {
                switch (layout) {
                case BufferLayout::Identity:
                    return "Identity";
                case BufferLayout::Undefined:
                    return "Undefined";
                case BufferLayout::UnorderedAccess:
                    return "UnorderedAccess";
                case BufferLayout::ReadOnly:
                    return "ReadOnly";
                case BufferLayout::TransferSrc:
                    return "TransferSrc";
                case BufferLayout::TransferDst:
                    return "TransferDst";
                default:
                    return "Unknown";
                }
            };

            static const auto ToHex = [](u64 value) -> eastl::string {
                char buf[32];
                sprintf(buf, "0x%016llX", value);
                return buf;
            };

            for (size_t i = 0; i < mBatches.size(); ++i) {
                const auto& batch = mBatches[i];
                out += "  Batch " + eastl::to_string(i) + ":\n";


                // Buffer Barriers
                out += "    Buffer Barriers:\n";
                for (const auto& bb : batch.bufferBarriers) {
                    auto name = mDevice->GetBufferInfo(bb.buffer).name;
                    out += "      Buffer: " + ToHex(eastl::bit_cast<u64>(bb.buffer)) + (name.empty() ? "" : (" {" + name + "}")) +
                           ", Region: [off=" + eastl::to_string(bb.region.offset) + ", sz=" +
                           ((bb.region.size == PYRO_MAX_SIZE) ? "WHOLE RANGE" : eastl::to_string(bb.region.size)) + "]" +
                           ", Layout: " + BufferLayoutToString(bb.srcLayout) + " -> " +
                           BufferLayoutToString(bb.dstLayout) + "\n";
                }

                // Image Barriers
                out += "    Image Barriers:\n";
                for (const auto& ib : batch.imageBarriers) {
                    auto name = mDevice->GetImageInfo(ib.image).name;
                    out += "      Image: " + ToHex(eastl::bit_cast<u64>(ib.image)) + (name.empty() ? "" : (" {" + name + "}")) +
                           ", Slice: [mip=(" + eastl::to_string(ib.imageSlice.baseMipLevel) + ";" + eastl::to_string(ib.imageSlice.baseMipLevel + ib.imageSlice.levelCount - 1) + "), " +
                           "arr=(" + eastl::to_string(ib.imageSlice.baseArrayLayer) + ";" + eastl::to_string(ib.imageSlice.baseArrayLayer + ib.imageSlice.layerCount - 1) + ")]" +
                           ", Layout: " + ImageLayoutToString(ib.srcLayout) + " -> " +
                           ImageLayoutToString(ib.dstLayout) + "\n";
                }

                // Task IDs
                out += "    Tasks:\n";
                for (auto id : batch.taskIds) {
                    auto name = mTasks[id]->GetTask()->Info().name;
                    out += "      " + eastl::to_string(id) + (name.empty() ? "" : (" {" + name + "}")) + "\n";
                }
            }

            return out;
        }
    } // namespace Renderer
} // namespace PyroshockStudios