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
#include "ShaderCompiler.hpp"
#include <PyroCommon/Core.hpp>
#include <PyroPlatform/Forward.hpp>
#include <PyroRHI/Api/Forward.hpp>
#include <ShockGraph/RHIManager.hpp>
#include <ShockGraph/Resources.hpp>
#include <ShockGraph/TaskGraph.hpp>
#include <ShockGraph/TaskResourceManager.hpp>

using namespace PyroshockStudios;
using namespace PyroshockStudios;
using namespace PyroshockStudios::Types;
using namespace PyroshockStudios::RHI;
using namespace PyroshockStudios::Platform;
namespace VisualTests {
    struct DisplayInfo {
        u32 width = 0;
        u32 height = 0;
    };
    struct CreateResourceInfo {
        const DisplayInfo& displayInfo;
        ShaderCompiler& shaderCompiler;
        TaskResourceManager& resourceManager;
    };
    struct ReleaseResourceInfo {
        TaskResourceManager& resourceManager;
    };
    struct IVisualTest {
        IVisualTest() = default;
        virtual ~IVisualTest() = default;

        PYRO_NODISCARD virtual eastl::string Title() const = 0;

        virtual void CreateResources(const CreateResourceInfo& info) = 0;
        virtual void ReleaseResources(const ReleaseResourceInfo& info) = 0;
        // Ownership is taken from these tests, do not try to access these tasks anymore inside IVisualTest
        PYRO_NODISCARD virtual eastl::span<GenericTask*> CreateTasks() = 0;

        PYRO_NODISCARD virtual bool TaskSupported(IDevice* device) = 0;

        PYRO_NODISCARD virtual bool UseTaskGraph() const = 0;
        PYRO_NODISCARD virtual TaskImage GetCompositeImageTaskGraph() = 0;
        PYRO_NODISCARD virtual Image GetCompositeImageRaw() = 0;
    };
} // namespace VisualTests