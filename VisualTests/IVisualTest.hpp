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

     PYRO_NODISCARD   virtual eastl::string Title() const = 0;

        virtual void CreateResources(const CreateResourceInfo& info) = 0;
        virtual void ReleaseResources(const ReleaseResourceInfo& info) = 0;
        // Ownership is taken from these tests, do not try to access these tasks anymore inside IVisualTest
       PYRO_NODISCARD virtual eastl::span<GenericTask*> CreateTasks() = 0;

       PYRO_NODISCARD virtual bool UseTaskGraph() const = 0;
       PYRO_NODISCARD virtual TaskImage GetCompositeImageTaskGraph() = 0;
       PYRO_NODISCARD virtual Image GetCompositeImageRaw() = 0;
    };
}