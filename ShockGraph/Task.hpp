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
#include <EASTL/array.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/optional.h>
#include <EASTL/string.h>
#include <PyroRHI/Api/Types.hpp>
#include <ShockGraph/Core.hpp>

namespace PyroshockStudios {
    inline namespace Renderer {
        using TaskAccessType = ::PyroshockStudios::RHI::Access;
        enum struct TaskType : u32 {
            None,
            Graphics,
            Compute,
            Transfer
        };
        class TaskGraph;
        class TaskCommandList;

        struct TaskBufferDependencyInfo {
            TaskBuffer buffer;
            TaskAccessType access;

            PYRO_NODISCARD PYRO_FORCEINLINE bool operator==(const TaskBufferDependencyInfo&) const = default;
            PYRO_NODISCARD PYRO_FORCEINLINE bool operator!=(const TaskBufferDependencyInfo&) const = default;
        };
        struct TaskImageDependencyInfo {
            TaskImage image;
            TaskAccessType access;

            PYRO_NODISCARD PYRO_FORCEINLINE bool operator==(const TaskImageDependencyInfo&) const = default;
            PYRO_NODISCARD PYRO_FORCEINLINE bool operator!=(const TaskImageDependencyInfo&) const = default;
        };
        struct TaskBlasDependencyInfo {
            TaskBlas blas;
            TaskAccessType access;

            PYRO_NODISCARD PYRO_FORCEINLINE bool operator==(const TaskBlasDependencyInfo&) const = default;
            PYRO_NODISCARD PYRO_FORCEINLINE bool operator!=(const TaskBlasDependencyInfo&) const = default;
        };
        struct TaskTlasDependencyInfo {
            TaskTlas tlas;
            TaskAccessType access;

            PYRO_NODISCARD PYRO_FORCEINLINE bool operator==(const TaskTlasDependencyInfo&) const = default;
            PYRO_NODISCARD PYRO_FORCEINLINE bool operator!=(const TaskTlasDependencyInfo&) const = default;
        };

        struct TaskInfo {
            eastl::string name = {};
            LabelColor color = {};

            PYRO_NODISCARD PYRO_FORCEINLINE bool operator==(const TaskInfo&) const = default;
            PYRO_NODISCARD PYRO_FORCEINLINE bool operator!=(const TaskInfo&) const = default;
        };


        class GenericTask : DeleteCopy, DeleteMove {
        public:
            SHOCKGRAPH_API GenericTask(const TaskInfo& info) : mTaskInfo(info) {}
            SHOCKGRAPH_API GenericTask() = default;
            SHOCKGRAPH_API virtual ~GenericTask() = default;

            virtual void SetupTask() = 0;
            virtual void ExecuteTask(TaskCommandList& commandList) = 0;

            PYRO_NODISCARD virtual PipelineBindPoint GetBindPoint() = 0;
            PYRO_NODISCARD virtual TaskType GetType() = 0;

            SHOCKGRAPH_API void UseBuffer(const TaskBufferDependencyInfo& info);
            SHOCKGRAPH_API void UseImage(const TaskImageDependencyInfo& info);
            SHOCKGRAPH_API void UseBlas(const TaskBlasDependencyInfo& info);
            SHOCKGRAPH_API void UseTlas(const TaskTlasDependencyInfo& info);

            PYRO_NODISCARD PYRO_FORCEINLINE const TaskInfo& Info() const {
                return mTaskInfo;
            }

        private:
            struct GenericSetup {
                eastl::vector<TaskBufferDependencyInfo> bufferDepends;
                eastl::vector<TaskImageDependencyInfo> imageDepends;
                eastl::vector<TaskBlasDependencyInfo> blasDepends;
                eastl::vector<TaskTlasDependencyInfo> tlasDepends;
            };

            GenericSetup mSetupData = {};
            TaskInfo mTaskInfo = {};

            friend class TaskGraph;
        };
        using TaskExecuteCallback = eastl::function<void(TaskCommandList&)>;


        class CustomTask : public virtual GenericTask {
        public:
            SHOCKGRAPH_API CustomTask() = default;
            SHOCKGRAPH_API virtual ~CustomTask() = default;

            virtual void ExecuteTask(ICommandBuffer* commandBuffer) = 0;

        private:
            PipelineBindPoint GetBindPoint() override { return PipelineBindPoint::None; }

            SHOCKGRAPH_API void ExecuteTask(TaskCommandList& commandList) override;
        };
        using TaskSetupCustomCallback = eastl::function<void(CustomTask&)>;
        using TaskExecuteCustomCallback = eastl::function<void(ICommandBuffer*)>;

        class CustomCallbackTask final : public CustomTask {
        public:
            SHOCKGRAPH_API CustomCallbackTask(const TaskInfo& info, TaskSetupCustomCallback&& setup, TaskExecuteCustomCallback&& exec, TaskType type)
                : GenericTask(info), mSetup(eastl::move(setup)), mExec(eastl::move(exec)), mType(type) {}
            SHOCKGRAPH_API ~CustomCallbackTask() = default;

            PYRO_FORCEINLINE void SetupTask() override { mSetup(static_cast<CustomTask&>(*this)); }
            PYRO_FORCEINLINE void ExecuteTask(ICommandBuffer* commandBuffer) override { mExec(commandBuffer); }

        private:
            TaskType GetType() override { return mType; }
            TaskSetupCustomCallback mSetup;
            TaskExecuteCustomCallback mExec;
            TaskType mType;
        };

        struct BindColorTargetInfo {
            TaskColorTarget target = {};
            eastl::optional<ColorClearValue> clear = eastl::nullopt;
            bool bBlending = {};
            eastl::optional<TaskColorTarget> resolve = eastl::nullopt;
        };
        struct BindDepthStencilTargetInfo {
            TaskDepthStencilTarget target = {};
            eastl::optional<float> depthClear = eastl::nullopt;
            eastl::optional<u32> stencilClear = eastl::nullopt;
            bool bReadOnly = {};
            bool bStencil = {};
            bool bDepth = {};
            bool bDepthStore = {};
            bool bStencilStore = {};
        };
        class GraphicsTask : public virtual GenericTask {
        public:
            SHOCKGRAPH_API GraphicsTask() = default;
            SHOCKGRAPH_API virtual ~GraphicsTask() = default;

            PYRO_NODISCARD PYRO_FORCEINLINE PipelineBindPoint GetBindPoint() final override {
                return PipelineBindPoint::Graphics;
            }
            PYRO_NODISCARD PYRO_FORCEINLINE TaskType GetType() final override {
                return TaskType::Graphics;
            }

            SHOCKGRAPH_API void BindColorTarget(const BindColorTargetInfo& info);
            SHOCKGRAPH_API void BindDepthStencilTarget(const BindDepthStencilTargetInfo& info);

        private:
            struct Setup {
                eastl::fixed_vector<BindColorTargetInfo, 8> colorTargets = {};
                eastl::optional<BindDepthStencilTargetInfo> depthStencilTarget = {};
            };

            Setup mGraphicsSetupData = {};
            friend class TaskGraph;
        };
        using TaskSetupGraphicsCallback = eastl::function<void(GraphicsTask&)>;

        class GraphicsCallbackTask final : public GraphicsTask {
        public:
            SHOCKGRAPH_API GraphicsCallbackTask(const TaskInfo& info, TaskSetupGraphicsCallback&& setup, TaskExecuteCallback&& exec)
                : GenericTask(info), mSetup(eastl::move(setup)), mExec(eastl::move(exec)) {}
            SHOCKGRAPH_API ~GraphicsCallbackTask() = default;

            PYRO_FORCEINLINE void SetupTask() override { mSetup(static_cast<GraphicsTask&>(*this)); }
            PYRO_FORCEINLINE void ExecuteTask(TaskCommandList& commandList) override { mExec(commandList); }

        private:
            TaskSetupGraphicsCallback mSetup;
            TaskExecuteCallback mExec;
        };

        class ComputeTask : public virtual GenericTask {
        public:
            SHOCKGRAPH_API ComputeTask() = default;
            SHOCKGRAPH_API virtual ~ComputeTask() = default;

            PYRO_NODISCARD PYRO_FORCEINLINE PipelineBindPoint GetBindPoint() final override {
                return PipelineBindPoint::Compute;
            }
            PYRO_NODISCARD PYRO_FORCEINLINE TaskType GetType() final override {
                return TaskType::Compute;
            }

        private:
            friend class TaskGraph;
        };
        using TaskSetupComputeCallback = eastl::function<void(ComputeTask&)>;

        class ComputeCallbackTask final : public ComputeTask {
        public:
            SHOCKGRAPH_API ComputeCallbackTask(const TaskInfo& info, TaskSetupComputeCallback&& setup, TaskExecuteCallback&& exec)
                : GenericTask(info), mSetup(eastl::move(setup)), mExec(eastl::move(exec)) {}
            SHOCKGRAPH_API ~ComputeCallbackTask() = default;

            PYRO_FORCEINLINE void SetupTask() override { mSetup(static_cast<ComputeTask&>(*this)); }
            PYRO_FORCEINLINE void ExecuteTask(TaskCommandList& commandList) override { mExec(commandList); }

        private:
            TaskSetupComputeCallback mSetup;
            TaskExecuteCallback mExec;
        };

        class TransferTask : public virtual GenericTask {
        public:
            SHOCKGRAPH_API TransferTask() = default;
            SHOCKGRAPH_API virtual ~TransferTask() = default;

            PYRO_NODISCARD PYRO_FORCEINLINE PipelineBindPoint GetBindPoint() final override {
                return PipelineBindPoint::None;
            }
            PYRO_NODISCARD PYRO_FORCEINLINE TaskType GetType() final override {
                return TaskType::Transfer;
            }

        private:
            friend class TaskGraph;
        };
        using TaskSetupTransferCallback = eastl::function<void(TransferTask&)>;

        class TransferCallbackTask final : public TransferTask {
        public:
            SHOCKGRAPH_API TransferCallbackTask(const TaskInfo& info, TaskSetupTransferCallback&& setup, TaskExecuteCallback&& exec)
                : GenericTask(info), mSetup(eastl::move(setup)), mExec(eastl::move(exec)) {}
            SHOCKGRAPH_API ~TransferCallbackTask() = default;

            PYRO_FORCEINLINE void SetupTask() override { mSetup(static_cast<TransferTask&>(*this)); }
            PYRO_FORCEINLINE void ExecuteTask(TaskCommandList& commandList) override { mExec(commandList); }

        private:
            TaskSetupTransferCallback mSetup;
            TaskExecuteCallback mExec;
        };

    } // namespace Renderer
} // namespace PyroshockStudios