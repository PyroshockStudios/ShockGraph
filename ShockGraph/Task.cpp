#include "Task.hpp"
#include "TaskCommandList.hpp"

#include <libassert/assert.hpp>
namespace PyroshockStudios {
    inline namespace Renderer {

        void GraphicsTask::BindColorTarget(const BindColorTargetInfo& info) {
            ASSERT(mGraphicsSetupData.colorTargets.size() < 8, "Trying to bind too many colour targets!");

            mGraphicsSetupData.colorTargets.push_back(info);

            Access access = {};
            if (info.bBlending) {
                access = AccessConsts::COLOR_ATTACHMENT_OUTPUT_READ_WRITE;
            } else {
                access = AccessConsts::COLOR_ATTACHMENT_OUTPUT_WRITE;
            }
            UseImage({ .image = info.target->Info().image, .access = access });
            if (info.resolve.has_value()) {
                // resolve requires colour attachment output
                UseImage({ .image = info.resolve.value()->Info().image, .access = AccessConsts::COLOR_ATTACHMENT_OUTPUT_WRITE });
            }
        }
        void GraphicsTask::BindDepthStencilTarget(const BindDepthStencilTargetInfo& info) {
            ASSERT(!mGraphicsSetupData.depthStencilTarget, "Already bound depth stencil target!");

            mGraphicsSetupData.depthStencilTarget.emplace(info);
            Access access = {};
            if (info.bReadOnly) {
                access = AccessConsts::EARLY_FRAGMENT_TESTS_READ | AccessConsts::LATE_FRAGMENT_TESTS_READ;
            } else {
                access = AccessConsts::EARLY_FRAGMENT_TESTS_READ_WRITE | AccessConsts::LATE_FRAGMENT_TESTS_READ_WRITE;
            }
            UseImage({ .image = info.target->Info().image, .access = access });
        }

        void GenericTask::UseBuffer(const TaskBufferDependencyInfo& info) {
            mSetupData.bufferDepends.emplace_back(info);
        }
        void GenericTask::UseImage(const TaskImageDependencyInfo& info) {
            mSetupData.imageDepends.emplace_back(info);
        }
        void CustomTask::ExecuteTask(TaskCommandList& commandList) {
            ExecuteTask(commandList.Internal());
        }
    }
}