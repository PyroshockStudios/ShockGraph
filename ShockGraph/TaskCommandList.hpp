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
#include <PyroRHI/Api/ICommandBuffer.hpp>

namespace PyroshockStudios {
    inline namespace Renderer {
        class TaskGraph;

        struct TaskCopyBufferInfo {
            TaskBufferRef srcBuffer;
            TaskBufferRef dstBuffer;
            DeviceSize srcOffset;
            DeviceSize dstOffset;
            DeviceSize size;
        };

        struct TaskCopyImageInfo {
            TaskImageRef srcImage;
            TaskImageRef dstImage;
            ImageArraySlice srcImageSlice = {};
            Offset3D srcOffset = {};
            ImageArraySlice dstImageSlice = {};
            Offset3D dstOffset = {};
            Extent3D extent = {};
        };

        using TaskClearUnorderedAccessViewInfo = ClearUnorderedAccessViewInfo;


        struct TaskUpdateBufferInfo {
            TaskBufferRef buffer;
            BufferRegion region = {};
            void* data = nullptr;
        };


        struct TaskSetUniformBufferViewInfo {
            u32 slot = {};
            TaskBufferRef buffer;
        };

        struct TaskSetUnorderedAccessViewInfo {
            u32 slot = {};
            UnorderedAccessId view = {};
        };

        struct TaskSetVertexBufferInfo {
            u32 slot = {};
            TaskBufferRef buffer;
            DeviceSize offset = {};
        };
        struct TaskSetIndexBufferInfo {
            TaskBufferRef buffer;
            DeviceSize offset = {};
            IndexType indexType = IndexType::Uint32;
        };

        using TaskDrawInfo = DrawInfo;
        using TaskDispatchInfo = DispatchInfo;

        struct TaskDrawIndirectInfo {
            TaskBufferRef indirectBuffer;
            usize indirectBufferOffset = {};
            u32 drawCount = 1;
            u32 drawCommandStride = sizeof(DrawArgumentBuffer);
        };
        struct TaskDrawIndexedIndirectInfo {
            TaskBufferRef indirectBuffer;
            usize indirectBufferOffset = {};
            u32 drawCount = 1;
            u32 drawCommandStride = sizeof(DrawIndexedArgumentBuffer);
        };

        using TaskDrawIndexedInfo = DrawIndexedInfo;

        struct TaskDispatchIndirectInfo {
            TaskBufferRef indirectBuffer;
            usize indirectBufferOffset = {};
        };

        class TaskCommandList : DeleteCopy, DeleteMove {
        public:
            PYRO_FORCEINLINE void CopyBuffer(const TaskCopyBufferInfo& info) {
                mCommandBuffer->CopyBufferToBuffer({
                    .srcBuffer = info.srcBuffer->Internal(),
                    .dstBuffer = info.dstBuffer->Internal(),
                    .srcOffset = info.srcOffset,
                    .dstOffset = info.dstOffset,
                    .size = info.size,
                });
            }
            PYRO_FORCEINLINE void CopyImage(const TaskCopyImageInfo& info) {
                mCommandBuffer->CopyImageToImage({
                    .srcImage = info.srcImage->Internal(),
                    .dstImage = info.dstImage->Internal(),
                    .srcImageSlice = info.srcImageSlice,
                    .srcOffset = info.srcOffset,
                    .dstImageSlice = info.dstImageSlice,
                    .dstOffset = info.dstOffset,
                });
            }
            PYRO_FORCEINLINE void ClearUnorderedAccessViw(const TaskClearUnorderedAccessViewInfo& info) {
                mCommandBuffer->ClearUnorderedAccessView(info);
            }
            PYRO_FORCEINLINE void UpdateBuffer(const TaskUpdateBufferInfo& info) {
                mCommandBuffer->UpdateBuffer({
                    .buffer = info.buffer->Internal(),
                    .region = info.region,
                    .data = info.data,
                });
            }

            template <StandardLayoutConcept T>
            PYRO_FORCEINLINE void PushConstant(const T& constant, const u32 offset = 0) {
                static_assert(sizeof(T) <= Limits::MAX_PUSH_CONSTANT_SIZE, "Push constant is too large! Please use a uniform buffer instead!");
                mCommandBuffer->PushConstant(constant, offset);
            }

            PYRO_FORCEINLINE void SetUniformBufferView(const TaskSetUniformBufferViewInfo& info) {
                mCommandBuffer->SetUniformBufferView({
                    .slot = info.slot,
                    .buffer = info.buffer->Internal(),
                    .bindPoint = mCurrBindPoint,
                });
            }
            PYRO_FORCEINLINE void SetUnorderedAccessView(const TaskSetUnorderedAccessViewInfo& info) {
                mCommandBuffer->SetUnorderedAccessView({
                    .slot = info.slot,
                    .view = info.view,
                    .bindPoint = mCurrBindPoint,
                });
            }
            PYRO_FORCEINLINE void SetRasterPipeline(TaskRasterPipelineRef pipeline) {
                RefreshPipelineIf(pipeline);
                mCommandBuffer->SetRasterPipeline(pipeline->Internal());
            }
            PYRO_FORCEINLINE void SetComputePipeline(TaskComputePipelineRef pipeline) {
                RefreshPipelineIf(pipeline);
                mCommandBuffer->SetComputePipeline(pipeline->Internal());
            }

            PYRO_FORCEINLINE void SetViewport(const ViewportInfo& info) {
                mCommandBuffer->SetViewport(info);
            }
            PYRO_FORCEINLINE void SetScissor(const Rect2D& info) {
                mCommandBuffer->SetScissor(info);
            }

            PYRO_FORCEINLINE void SetVertexBuffer(const TaskSetVertexBufferInfo& info) {
                mCommandBuffer->SetVertexBuffer({
                    .slot = info.slot,
                    .buffer = info.buffer->Internal(),
                    .offset = info.offset,
                });
            }
            PYRO_FORCEINLINE void SetIndexBuffer(const TaskSetIndexBufferInfo& info) {
                mCommandBuffer->SetIndexBuffer({
                    .buffer = info.buffer->Internal(),
                    .offset = info.offset,
                    .indexType = info.indexType,
                });
            }

            PYRO_FORCEINLINE void Draw(const TaskDrawInfo& info) {
                mCommandBuffer->Draw(info);
            }
            PYRO_FORCEINLINE void DrawIndexed(const TaskDrawIndexedInfo& info) {
                mCommandBuffer->DrawIndexed(info);
            }
            PYRO_FORCEINLINE void DrawIndirect(const TaskDrawIndirectInfo& info) {
                mCommandBuffer->DrawIndirect({
                    .indirectBuffer = info.indirectBuffer->Internal(),
                    .indirectBufferOffset = info.indirectBufferOffset,
                    .drawCount = info.drawCount,
                    .drawCommandStride = info.drawCommandStride,
                });
            }
            PYRO_FORCEINLINE void DrawIndexedIndirect(const TaskDrawIndexedIndirectInfo& info) {
                mCommandBuffer->DrawIndexedIndirect({
                    .indirectBuffer = info.indirectBuffer->Internal(),
                    .indirectBufferOffset = info.indirectBufferOffset,
                    .drawCount = info.drawCount,
                    .drawCommandStride = info.drawCommandStride,
                });
            }

            PYRO_FORCEINLINE void Dispatch(const TaskDispatchInfo& info) {
                mCommandBuffer->Dispatch(info);
            }
            PYRO_FORCEINLINE void DispatchIndirect(const TaskDispatchIndirectInfo& info) {
                mCommandBuffer->DrawIndexedIndirect({
                    .indirectBuffer = info.indirectBuffer->Internal(),
                    .indirectBufferOffset = info.indirectBufferOffset,
                });
            }

            PYRO_FORCEINLINE ICommandBuffer* Internal() {
                return mCommandBuffer;
            }

        private:
            template <typename Pipeline>
            PYRO_FORCEINLINE void RefreshPipelineIf(Pipeline& pipeline) {
                if (!pipeline->mbDirty)
                    return;
                pipeline->mbDirty = false;
                mCommandBuffer->DestroyDeferred(pipeline->mPipeline);
                pipeline->Recreate();
            }

            PipelineBindPoint mCurrBindPoint = {};
            ICommandBuffer* mCommandBuffer = nullptr;
            IDevice* mOwningDevice = nullptr;

            friend class TaskGraph;
        };
    } // namespace Renderer
} // namespace PyroshockStudios