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

#include <PyroCommon/Core.hpp>
#include <PyroCommon/Memory.hpp>
#ifdef SHOCKGRAPH_USE_PYRO_PLATFORM
#include <PyroPlatform/Forward.hpp>
#endif
#include <PyroRHI/Api/Forward.hpp>
#include <PyroRHI/Api/GPUResource.hpp>
#include <PyroRHI/Api/Pipeline.hpp>
#include <PyroRHI/Api/RenderTarget.hpp>
#include <PyroRHI/Api/Types.hpp>
#include <PyroRHI/Shader/ShaderProgram.hpp>
#include <ShockGraph/Core.hpp>

namespace PyroshockStudios {
    inline namespace Renderer {
        using ResourceIndex = u32;
        class TaskCommandList;
        class TaskResourceManager;
        class ShaderReloadListener;


        struct TaskResource_ : public RefCounted, DeleteCopy, DeleteMove {
            SHOCKGRAPH_API TaskResource_(TaskResourceManager* owner);
            SHOCKGRAPH_API virtual ~TaskResource_();

            PYRO_NODISCARD PYRO_FORCEINLINE u32 GetId() const {
                return mId;
            }

        protected:
            IDevice* Device();
            PYRO_FORCEINLINE TaskResourceManager* Owner() {
                return mOwner;
            }

        private:
            u32 mId = ~(0U);
            TaskResourceManager* mOwner = nullptr;
            friend class TaskResourceManager;
        };
        using TaskResource = SharedRef<TaskResource_>;

        struct TaskRasterPipeline_;
        struct TaskComputePipeline_;

        struct TaskShader_ final : public RefCounted, DeleteCopy, DeleteMove {
            SHOCKGRAPH_API TaskShader_(ShaderProgram&& program, FunctionPtr<void(void*, TaskShader_*)> deleter, void* deleterUserData);
            SHOCKGRAPH_API ~TaskShader_() override;

            PYRO_NODISCARD PYRO_FORCEINLINE const ShaderProgram& Program() const { return mProgram; }

        private:
            PYRO_FORCEINLINE void RemoveReference(TaskResource_* r) {
                mUsedByResources.erase(eastl::find(mUsedByResources.begin(), mUsedByResources.end(), r));
            }

            ShaderProgram mProgram = {};
            eastl::vector<TaskResource_*> mUsedByResources = {};
            void* mDeleterUserData = {};
            FunctionPtr<void(void*, TaskShader_*)> mDeleter = {};

            friend class TaskResourceManager;
            friend class ShaderReloadListener;
            friend struct TaskRasterPipeline_;
            friend struct TaskComputePipeline_;
        };
        using TaskShader = SharedRef<TaskShader_>;

        struct TaskShaderInfo {
            /**
             * @brief Shader program
             */
            TaskShader program = {};

            /**
             * @brief List of specialization constants to set at pipeline creation time.
             */
            eastl::span<const SpecializationConstantInfo> specializationConstants = {};

            PYRO_NODISCARD bool operator==(const TaskShaderInfo&) const = default;
            PYRO_NODISCARD bool operator!=(const TaskShaderInfo&) const = default;
        };

        using TaskRasterPipelineInfo = RasterPipelineInfo;


        struct TaskRasterPipelineShaders {
            /**
             * @brief Vertex shader stage configuration.
             */
            eastl::optional<TaskShaderInfo> vertexShaderInfo = {};

            /**
             * @brief Hull (tessellation control) shader stage configuration.
             */
            eastl::optional<TaskShaderInfo> hullShaderInfo = {};

            /**
             * @brief Domain (tessellation evaluation) shader stage configuration.
             */
            eastl::optional<TaskShaderInfo> domainShaderInfo = {};

            /**
             * @brief Geometry shader stage configuration.
             */
            eastl::optional<TaskShaderInfo> geometryShaderInfo = {};

            /**
             * @brief Fragment (pixel) shader stage configuration.
             */
            eastl::optional<TaskShaderInfo> fragmentShaderInfo = {};

            PYRO_NODISCARD bool operator==(const TaskRasterPipelineShaders&) const = default;
            PYRO_NODISCARD bool operator!=(const TaskRasterPipelineShaders&) const = default;
        };
        struct TaskRasterPipeline_ final : public TaskResource_ {
            SHOCKGRAPH_API TaskRasterPipeline_(TaskResourceManager* owner, const TaskRasterPipelineInfo& info, const TaskRasterPipelineShaders& shaderStages);
            SHOCKGRAPH_API ~TaskRasterPipeline_() override;
            PYRO_NODISCARD PYRO_FORCEINLINE RasterPipeline Internal() {
                return mPipeline;
            }

            PYRO_NODISCARD PYRO_FORCEINLINE const TaskRasterPipelineInfo& Info() const { return mInfo; }

        private:
            void Recreate();
            RasterPipeline mPipeline = nullptr;
            TaskRasterPipelineInfo mInfo;
            TaskRasterPipelineShaders mStages;
            bool mbDirty = false;
            friend class TaskCommandList;
            friend class TaskResourceManager;
            friend class ShaderReloadListener;
        };
        using TaskRasterPipeline = SharedRef<TaskRasterPipeline_>;
        using TaskRasterPipelineRef = TaskRasterPipeline&;

        using TaskComputePipelineInfo = ComputePipelineInfo;
        struct TaskComputePipeline_ final : public TaskResource_ {
            SHOCKGRAPH_API TaskComputePipeline_(TaskResourceManager* owner, const TaskComputePipelineInfo& info, const TaskShaderInfo& shader);
            SHOCKGRAPH_API ~TaskComputePipeline_() override;
            PYRO_NODISCARD PYRO_FORCEINLINE ComputePipeline Internal() {
                return mPipeline;
            }

            PYRO_NODISCARD PYRO_FORCEINLINE const TaskComputePipelineInfo& Info() const { return mInfo; }

        private:
            void Recreate();
            ComputePipeline mPipeline = nullptr;
            TaskComputePipelineInfo mInfo;
            TaskShaderInfo mShader;
            bool mbDirty = false;
            friend class TaskCommandList;
            friend class TaskResourceManager;
            friend class ShaderReloadListener;
        };
        using TaskComputePipeline = SharedRef<TaskComputePipeline_>;

        using TaskComputePipelineRef = TaskComputePipeline&;

        struct TaskBufferInfo {
            usize size = 0;
            BufferUsageFlags usage = {};
            // Buffer is stored on CPU visible memory
            bool bCpuVisible = false;
            // Buffer can be read from CPU
            bool bReadback = false;
            // Buffer is reliably accessible between CPU and GPU
            bool bDynamic = false;
            eastl::string name = {};
        };
        struct TaskBuffer_ final : public TaskResource_ {
            SHOCKGRAPH_API TaskBuffer_(TaskResourceManager* owner, const TaskBufferInfo& info, Buffer&& buffer, eastl::vector<Buffer>&& inFlightBuffers);
            SHOCKGRAPH_API ~TaskBuffer_() override;
            PYRO_NODISCARD PYRO_FORCEINLINE Buffer Internal() {
                return mBuffer;
            }
            PYRO_NODISCARD PYRO_FORCEINLINE Buffer InternalInFlightBuffer(u32 index) {
                return mInFlightBuffers[index];
            }
            PYRO_NODISCARD PYRO_FORCEINLINE const TaskBufferInfo& Info() const { return mInfo; }

            // TODO: maybe a WriteMemory function, or a Flush function to allow for better copy caching?
            PYRO_NODISCARD SHOCKGRAPH_API u8* MappedMemory();

        private:
            Buffer mBuffer = PYRO_NULL_BUFFER;
            eastl::vector<Buffer> mInFlightBuffers = {};
            u32 mCurrentBufferInFlight = 0;

            TaskBufferInfo mInfo;

            friend class TaskResourceManager;
            friend class TaskGraph;
        };
        using TaskBuffer = SharedRef<TaskBuffer_>;
        using TaskBufferRef = TaskBuffer&;

        struct TaskImageInfo {
            ImageDimensions dimensions = ImageDimensions::e2D;
            Format format = Format::RGBA8Unorm;
            Extent3D size = {};
            u32 mipLevelCount = 1;
            u32 arrayLayerCount = 1;
            RasterizationSamples sampleCount = RasterizationSamples::e1;
            ImageUsageFlags usage = {};
            eastl::string name = {};
        };
        struct TaskImage_ final : public TaskResource_ {
            SHOCKGRAPH_API TaskImage_(TaskResourceManager* owner, const TaskImageInfo& info, Image&& image);
            SHOCKGRAPH_API ~TaskImage_() override;
            PYRO_NODISCARD PYRO_FORCEINLINE Image Internal() {
                return mImage;
            }

            PYRO_NODISCARD PYRO_FORCEINLINE const TaskImageInfo& Info() const { return mInfo; }
            PYRO_NODISCARD PYRO_FORCEINLINE ImageMipArraySlice Slice() const {
                return {
                    .baseMipLevel = 0,
                    .levelCount = mInfo.mipLevelCount,
                    .baseArrayLayer = 0,
                    .layerCount = mInfo.arrayLayerCount
                };
            }

        private:
            Image mImage = PYRO_NULL_IMAGE;
            TaskImageInfo mInfo;
        };
        using TaskImage = SharedRef<TaskImage_>;
        using TaskImageRef = TaskImage&;

        struct TaskColorTargetInfo {
            TaskImage image;
            ImageSlice slice;
            eastl::string name = {};
        };
        struct TaskColorTarget_ final : public TaskResource_ {
            SHOCKGRAPH_API TaskColorTarget_(TaskResourceManager* owner, const TaskColorTargetInfo& info, RenderTarget&& renderTarget);
            SHOCKGRAPH_API ~TaskColorTarget_() override;
            PYRO_NODISCARD PYRO_FORCEINLINE RenderTarget Internal() {
                return mRenderTarget;
            }

            PYRO_NODISCARD PYRO_FORCEINLINE const TaskColorTargetInfo& Info() const { return mInfo; }

        private:
            RenderTarget mRenderTarget = nullptr;
            TaskColorTargetInfo mInfo;
        };
        using TaskColorTargetRef = TaskColorTarget_&;
        using TaskColorTarget = SharedRef<TaskColorTarget_>;

        struct TaskDepthStencilTargetInfo {
            TaskImage image;
            ImageSlice slice;
            bool bDepth = {};
            bool bStencil = {};
            eastl::string name = {};
        };
        struct TaskDepthStencilTarget_ final : public TaskResource_ {
            SHOCKGRAPH_API TaskDepthStencilTarget_(TaskResourceManager* owner, const TaskDepthStencilTargetInfo& info, RenderTarget&& renderTarget);
            SHOCKGRAPH_API ~TaskDepthStencilTarget_() override;
            PYRO_NODISCARD PYRO_FORCEINLINE RenderTarget Internal() {
                return mRenderTarget;
            }

            PYRO_NODISCARD PYRO_FORCEINLINE const TaskDepthStencilTargetInfo& Info() const { return mInfo; }

        private:
            RenderTarget mRenderTarget = nullptr;
            TaskDepthStencilTargetInfo mInfo;
        };
        using TaskDepthStencilTarget = SharedRef<TaskDepthStencilTarget_>;
        using TaskDepthStencilTargetRef = TaskDepthStencilTarget&;

        enum class TaskSwapChainFormat {
            e8Bit,
            e10Bit,
            e16BitHDR,
        };
        struct TaskSwapChainInfo {
#ifdef SHOCKGRAPH_USE_PYRO_PLATFORM
            IWindow* window = nullptr;
#else
            NativeHandle nativeWindow = {};
            NativeHandle nativeInstance = {};
            Extent2D nativeWindowExtent = {};
#endif
            TaskSwapChainFormat format = TaskSwapChainFormat::e8Bit;
            ImageUsageFlags imageUsage = ImageUsageFlagBits::NONE;
            bool vsync = true;
            eastl::string name = {};
        };
        struct TaskSwapChain_ final : public TaskResource_ {
            SHOCKGRAPH_API TaskSwapChain_(TaskResourceManager* owner, const TaskSwapChainInfo& info, ISwapChain*&& swapChain);
            SHOCKGRAPH_API ~TaskSwapChain_() override;
            PYRO_NODISCARD PYRO_FORCEINLINE ISwapChain* Internal() {
                return mSwapChain;
            }
            PYRO_NODISCARD PYRO_FORCEINLINE const TaskSwapChainInfo& Info() const { return mInfo; }
            PYRO_FORCEINLINE void Resize() { bFlagResize = true; }

        private:
            ISwapChain* mSwapChain = nullptr;
            TaskSwapChainInfo mInfo;
            bool bFlagResize = false;
            friend class TaskResourceManager;
            friend class TaskGraph;
        };
        using TaskSwapChain = SharedRef<TaskSwapChain_>;
        using TaskSwapChainRef = TaskSwapChain&;
    } // namespace Renderer
} // namespace PyroshockStudios