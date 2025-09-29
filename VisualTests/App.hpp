#pragma once
#include "IVisualTest.hpp"
#include <PyroCommon/Core.hpp>
#include <PyroPlatform/Forward.hpp>
#include <PyroRHI/Api/Forward.hpp>
#include <ShockGraph/RHIManager.hpp>

#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>
using namespace PyroshockStudios;
using namespace PyroshockStudios::Types;
using namespace PyroshockStudios::RHI;
using namespace PyroshockStudios::Platform;
namespace VisualTests {
    class VisualTestApp {
    public:
        VisualTestApp();
        ~VisualTestApp();

        template <typename Test>
        void RegisterTest() {
            AddTest(eastl::make_unique<Test>());
        }
        void Run();

    private:
        void AddTest(eastl::unique_ptr<IVisualTest>&& test);
        void NextRHI();
        void PrevRHI();
        void NextTest();
        void PrevTest();

    private:
        void ReleaseTaskResources();

        void CreateWindow();

        void CreateRHI();
        void UpdateTitle();

        void RebuildTaskGraph();

        // TEST_NAME | Test N / M | RHI RHI_NAME | FPS K
        eastl::string GetFullTestTitle();

        IVisualTest* ActiveTest() {
            return mTests[mCurrentTest].get();
        }

    private:
        IWindow* mActiveWindow = nullptr;

        eastl::unique_ptr<RHIManager> mRHIManager = nullptr;

    private:
        TaskSwapChain mSwapChain = nullptr;


        eastl::unique_ptr<ShaderCompiler> mShaderCompiler = nullptr;
        eastl::unique_ptr<TaskResourceManager> mTaskResourceManager = nullptr;
        eastl::unique_ptr<TaskGraph> mTaskRenderGraph = nullptr;

    private:
        u32 mFailedRHICreationAttempts = 0;
        u32 mCurrentRHI = 0;
        u32 mCurrentTest = 0;
        eastl::vector<eastl::unique_ptr<IVisualTest>> mTests;
        eastl::vector<eastl::unique_ptr<GenericTask>> mCurrentTestTasks;

    private:
        float mLastDeltaTime = 0.0f;
        bool bDebugLayers = true;
    };
}