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

        struct TaskInfo {
            eastl::string name = {};
            LabelColor color = {};

            PYRO_NODISCARD PYRO_FORCEINLINE bool operator==(const TaskInfo&) const = default;
            PYRO_NODISCARD PYRO_FORCEINLINE bool operator!=(const TaskInfo&) const = default;
        };


        class GenericTask : DeleteCopy, DeleteMove {
        public:
            GenericTask(const TaskInfo& info) : mTaskInfo(info) {
            }
            GenericTask() = default;
            virtual ~GenericTask() = default;

            virtual void SetupTask() = 0;
            virtual void ExecuteTask(TaskCommandList& commandList) = 0;

            PYRO_NODISCARD virtual PipelineBindPoint GetBindPoint() = 0;
            PYRO_NODISCARD virtual TaskType GetType() = 0;

            SHOCKGRAPH_API void UseBuffer(const TaskBufferDependencyInfo& info);
            SHOCKGRAPH_API void UseImage(const TaskImageDependencyInfo& info);

            PYRO_NODISCARD PYRO_FORCEINLINE const TaskInfo& Info() const {
                return mTaskInfo;
            }

        private:
            struct GenericSetup {
                eastl::vector<TaskBufferDependencyInfo> bufferDepends;
                eastl::vector<TaskImageDependencyInfo> imageDepends;
            };

            GenericSetup mSetupData = {};
            TaskInfo mTaskInfo = {};

            friend class TaskGraph;
        };
        using TaskExecuteCallback = eastl::function<void(TaskCommandList&)>;


        class CustomTask : public virtual GenericTask {
        public:
            CustomTask() = default;
            virtual ~CustomTask() = default;

            virtual void ExecuteTask(ICommandBuffer* commandBuffer) = 0;

        private:
            PipelineBindPoint GetBindPoint() override { return PipelineBindPoint::None; }

            void ExecuteTask(TaskCommandList& commandList) override;
        };
        using TaskSetupCustomCallback = eastl::function<void(CustomTask&)>;
        using TaskExecuteCustomCallback = eastl::function<void(ICommandBuffer*)>;

        class CustomCallbackTask final : public CustomTask {
        public:
            CustomCallbackTask(const TaskInfo& info, TaskSetupCustomCallback&& setup, TaskExecuteCustomCallback&& exec, TaskType type)
                : GenericTask(info), mSetup(eastl::move(setup)), mExec(eastl::move(exec)), mType(type) {}
            ~CustomCallbackTask() = default;

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
            GraphicsTask() = default;
            virtual ~GraphicsTask() = default;

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
            GraphicsCallbackTask(const TaskInfo& info, TaskSetupGraphicsCallback&& setup, TaskExecuteCallback&& exec)
                : GenericTask(info), mSetup(eastl::move(setup)), mExec(eastl::move(exec)) {}
            ~GraphicsCallbackTask() = default;

            PYRO_FORCEINLINE void SetupTask() override { mSetup(static_cast<GraphicsTask&>(*this)); }
            PYRO_FORCEINLINE void ExecuteTask(TaskCommandList& commandList) override { mExec(commandList); }

        private:
            TaskSetupGraphicsCallback mSetup;
            TaskExecuteCallback mExec;
        };

        class ComputeTask : public virtual GenericTask {
        public:
            ComputeTask() = default;
            virtual ~ComputeTask() = default;

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
            ComputeCallbackTask(const TaskInfo& info, TaskSetupComputeCallback&& setup, TaskExecuteCallback&& exec)
                : GenericTask(info), mSetup(eastl::move(setup)), mExec(eastl::move(exec)) {}
            ~ComputeCallbackTask() = default;

            void SetupTask() override { mSetup(static_cast<ComputeTask&>(*this)); }
            void ExecuteTask(TaskCommandList& commandList) override { mExec(commandList); }

        private:
            TaskSetupComputeCallback mSetup;
            TaskExecuteCallback mExec;
        };

        class TransferTask : public virtual GenericTask {
        public:
            TransferTask() = default;
            virtual ~TransferTask() = default;

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
            TransferCallbackTask(const TaskInfo& info, TaskSetupTransferCallback&& setup, TaskExecuteCallback&& exec)
                : GenericTask(info), mSetup(eastl::move(setup)), mExec(eastl::move(exec)) {}
            ~TransferCallbackTask() = default;

            PYRO_FORCEINLINE void SetupTask() override { mSetup(static_cast<TransferTask&>(*this)); }
            PYRO_FORCEINLINE void ExecuteTask(TaskCommandList& commandList) override { mExec(commandList); }

        private:
            TaskSetupTransferCallback mSetup;
            TaskExecuteCallback mExec;
        };

    }
}