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

#include "Task.hpp"
#include "TaskCommandList.hpp"

#include <libassert/assert.hpp>
namespace PyroshockStudios {
    inline namespace Renderer {

        SHOCKGRAPH_API void GraphicsTask::BindColorTarget(const BindColorTargetInfo& info) {
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
        SHOCKGRAPH_API void GraphicsTask::BindDepthStencilTarget(const BindDepthStencilTargetInfo& info) {
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

        SHOCKGRAPH_API void GenericTask::UseBuffer(const TaskBufferDependencyInfo& info) {
            mSetupData.bufferDepends.emplace_back(info);
        }
        SHOCKGRAPH_API void GenericTask::UseImage(const TaskImageDependencyInfo& info) {
            mSetupData.imageDepends.emplace_back(info);
        }
        SHOCKGRAPH_API void CustomTask::ExecuteTask(TaskCommandList& commandList) {
            ExecuteTask(commandList.Internal());
        }
    } // namespace Renderer
} // namespace PyroshockStudios