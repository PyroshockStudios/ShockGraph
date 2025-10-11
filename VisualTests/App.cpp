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

#include "App.hpp"
#include <PyroCommon/Logger.hpp>
#include <PyroPlatform/Factory.hpp>
#include <PyroPlatform/Time/IClock.hpp>
#include <PyroPlatform/Window/IWindow.hpp>
#include <PyroPlatform/Window/IWindowInput.hpp>
#include <PyroPlatform/Window/IWindowManager.hpp>
#include <PyroPlatform/Window/WindowEvents.hpp>
#include <PyroRHI/Api/IDevice.hpp>
#include <PyroRHI/Context.hpp>
#include <libassert/assert.hpp>
namespace VisualTests {
    ILogStream* gPlatformSink = nullptr;
    ILogStream* gRHILoaderSink = nullptr;
    ILogStream* gRHISink = nullptr;
    ILogStream* gSGSink = nullptr;
    ILogStream* gShaderSink = nullptr;


    constexpr u32 FRAMES_IN_FLIGHT = 3;
    constexpr u32 WIDTH = 1000;
    constexpr u32 HEIGHT = 700;
    constexpr bool USE_VSYNC = true;
    constexpr bool ENABLE_DEBUG_LAYERS = true;

    VisualTestApp::VisualTestApp() {
        gPlatformSink = new StdoutLogger("PLATFORM");
        gSGSink = new StdoutLogger("TASKGRAPH");
        gRHILoaderSink = new StdoutLogger("RHILOADER");
        gShaderSink = new StdoutLogger("SLANGCOMPILER");

        PlatformFactory::Get<IWindowManager>()->InjectLogger(gPlatformSink);
        if (!PlatformFactory::Get<IWindowManager>()->Init()) {
            abort();
        }
    }

    VisualTestApp::~VisualTestApp() {
        ReleaseTaskResources();
        mSwapChain = nullptr;
        PlatformFactory::Get<IWindowManager>()->DestroyWindow(mActiveWindow);
        PlatformFactory::Get<IWindowManager>()->Terminate();

        mTests.clear();
        mCurrentTestTasks.clear();
        mShaderCompiler = nullptr;
        mTaskResourceManager = nullptr;
        mTaskRenderGraph = nullptr;
        mRHIManager = nullptr;
        delete gPlatformSink;
        delete gSGSink;
        delete gRHILoaderSink;
        delete gShaderSink;
        if (gRHISink) {
            delete gRHISink;
        }
    }

    void VisualTestApp::AddTest(eastl::unique_ptr<IVisualTest>&& test) {
        mTests.emplace_back(eastl::move(test));
    }

    void VisualTestApp::Run() {
        ASSERT(!mTests.empty(), "Add tests before running the visual test app!");
        printf("Running Visual Tests with %i tests\n", (int)mTests.size());
        CreateRHI();
        RebuildTaskGraph();

        f64 last = PlatformFactory::Get<IClock>()->GetTimeElapsed();
        while (!mActiveWindow->ShouldClose()) {
            f64 next = PlatformFactory::Get<IClock>()->GetTimeElapsed();
            mLastDeltaTime = static_cast<f32>(next - last);
            last = next;
            UpdateTitle();
            PlatformFactory::Get<IWindowManager>()->PollEvents();

            mTaskRenderGraph->BeginFrame();
            mTaskRenderGraph->Execute();
            mTaskRenderGraph->EndFrame();
        }
    }

    void VisualTestApp::NextRHI() {
        ReleaseTaskResources();
        u32 numRhis = static_cast<u32>(mRHIManager->QueryAvailableRHIs().size());
        mCurrentRHI = (mCurrentRHI + 1) % numRhis;
        CreateRHI();
        RebuildTaskGraph();
    }
    void VisualTestApp::PrevRHI() {
        ReleaseTaskResources();
        u32 numRhis = static_cast<u32>(mRHIManager->QueryAvailableRHIs().size());
        mCurrentRHI = (mCurrentRHI + numRhis - 1) % numRhis;
        CreateRHI();
        RebuildTaskGraph();
    }

    void VisualTestApp::NextTest() {
        ReleaseTaskResources();
        u32 numTests = static_cast<u32>(mTests.size());
        mCurrentTest = (mCurrentTest + 1) % numTests;
        RebuildTaskGraph();
    }

    void VisualTestApp::PrevTest() {
        ReleaseTaskResources();
        u32 numTests = static_cast<u32>(mTests.size());
        mCurrentTest = (mCurrentTest + numTests - 1) % numTests;
        RebuildTaskGraph();
    }

    void VisualTestApp::ReleaseTaskResources() {
        if (mRHIManager->GetRHIDevice())
            mRHIManager->GetRHIDevice()->WaitIdle();
        mCurrentTestTasks.clear();
        if (ActiveTest())
            ActiveTest()->ReleaseResources({ .resourceManager = *mTaskResourceManager });
        if (mTaskRenderGraph)
            mTaskRenderGraph->Reset();
    }

    void VisualTestApp::CreateWindow() {
        Point oldPos = { -1, -1 };
        if (mActiveWindow) {
            oldPos = mActiveWindow->GetPosition();
            PlatformFactory::Get<IWindowManager>()->DestroyWindow(mActiveWindow);
        }
        mActiveWindow = PlatformFactory::Get<IWindowManager>()->CreateWindow({
            .width = WIDTH,
            .height = HEIGHT,
            .title = "Visual Tests",
            .flags = WindowCreateBits::VISIBLE | WindowCreateBits::DECORATED | WindowCreateBits::FOCUSED,
        });
        if (oldPos != Point{ -1, -1 }) {
            mActiveWindow->SetPosition(oldPos);
        }
        auto handler = mActiveWindow->GetInputHandler()->GetEvents().BindEvent<KeyEvent>([this](KeyEvent& e) {
            if (!e.kbDown) {
                return;
            }
            switch (e.kKey) {
            case KeyCode::Minus:
                PrevRHI();
                break;
            case KeyCode::Equal:
                NextRHI();
                break;
            case KeyCode::Left:
                PrevTest();
                break;
            case KeyCode::Right:
                NextTest();
                break;
            case KeyCode::KeyP: {
                Logger::Info(gSGSink, "Task Graph Timings:");
                for (GenericTask* task : mTaskRenderGraph->GetTasks()) {
                    f64 ns = mTaskRenderGraph->GetTaskTimingsNs(task);
                    f64 ms = ns / 1e6;
                    Logger::Info(gSGSink, "    {}: {:.5f} ms", task->Info().name, ms);
                }
                Logger::Info(gSGSink, "-- GRAPH FLUSHES TIMING -- {:.5f} ms", mTaskRenderGraph->GetMiscFlushesTimingsNs() / 1e6);
                Logger::Info(gSGSink, "--  TOTAL GRAPH TIMING  -- {:.5f} ms", mTaskRenderGraph->GetGraphTimingsNs() / 1e6);
            } break;
            }
        });
    }

    void VisualTestApp::CreateRHI() {
        mSwapChain = nullptr;
        mTaskRenderGraph = nullptr;
        mTaskResourceManager = nullptr;

        // because of the vulkan swapchain being so limited
        // dxgi is latched forever on the window and cannot be detached
        // so the window must be recreated
        if (!mActiveWindow || mRHIManager && strcmp(mRHIManager->GetAttachedRHIInfo().info.shorthand, "dx12") == 0) {
            CreateWindow();
        }

        mRHIManager = eastl::make_unique<RHIManager>();
        mRHIManager->InjectLogger(gRHILoaderSink);
        mRHIManager->DiscoverAvailableRHIs();

        GUID useRHI = GUID::Invalid();
        RHICreateInfo createInfo{};
        RHIInfo useRHIInfo = {};
        u32 rhiIndex = 0;
        for (const auto& rhiInfo : mRHIManager->QueryAvailableRHIs()) {
            if (rhiIndex == mCurrentRHI) {
                useRHI = rhiInfo.guid;
                useRHIInfo = rhiInfo;
                u32 optionIndex = 0;
                for (u32 i = 0; i < PYRO_RHI_MAX_OPTIONS; ++i) {
                    const auto& option = rhiInfo.availableOptions[i];
                    if (ENABLE_DEBUG_LAYERS) {
                        if (strcmp(option.name, "debug") == 0) {
                            createInfo.options[optionIndex++].optionIndex = i;
                        }
                    }
                }
                break;
            }
            ++rhiIndex;
        }
        if (useRHI.Valid()) {
            createInfo.appName = "Visual Test App";
            createInfo.appVersion = BUILD_VERSION;
            createInfo.engineVersion = BUILD_VERSION;
            createInfo.engineName = "ShockGraph Visual Test";
            if (gRHISink)
                delete gRHISink;
            gRHISink = new StdoutLogger(useRHIInfo.shorthand);
            createInfo.pLoggerSink = gRHISink;
            if (!mRHIManager->AttachRHI(useRHI, createInfo)) {
                Logger::Warn(gRHILoaderSink, "Target RHI failed to attach, trying to attach next best RHI");
                u32 numRhis = static_cast<u32>(mRHIManager->QueryAvailableRHIs().size());
                if (mFailedRHICreationAttempts == numRhis) {
                    Logger::Fatal(gRHILoaderSink, "Too many attempts failed at attaching RHIs, aborting...");
                }
                mCurrentRHI = (mCurrentRHI + 1) % numRhis;
                ++mFailedRHICreationAttempts;
                CreateRHI();
                return;
            }
        } else {
            Logger::Fatal(gRHILoaderSink, "Failed to find a suitable RHI!");
        }
        mFailedRHICreationAttempts = 0;
        mTaskResourceManager = eastl::make_unique<TaskResourceManager>(TaskResourceManagerInfo{
            .rhi = mRHIManager->GetAttachedRHI(),
            .device = mRHIManager->GetRHIDevice(),
            .framesInFlight = FRAMES_IN_FLIGHT });
        mTaskResourceManager->InjectLogger(gSGSink);
        mTaskRenderGraph = eastl::make_unique<TaskGraph>(TaskGraphInfo{
            .resourceManager = mTaskResourceManager.get() });
        mTaskRenderGraph->InjectLogger(gSGSink);
        mSwapChain = mTaskResourceManager->CreateSwapChain({
            .window = mActiveWindow,
            .format = TaskSwapChainFormat::e8Bit,
            .imageUsage = ImageUsageFlagBits::TRANSFER_DST | ImageUsageFlagBits::BLIT_DST,
            .vsync = USE_VSYNC,
            .name = "Visual Test Swap Chain",
        });
        mShaderCompiler = nullptr;
        mShaderCompiler = eastl::make_unique<ShaderCompiler>(*mTaskResourceManager, mRHIManager->GetAttachedRHI()->ShaderFeatureSet());
    }

    void VisualTestApp::UpdateTitle() {
        mActiveWindow->SetTitle("SW Visual Tests (Cycle Test: <>, Cycle RHI: -+) " +
                                GetFullTestTitle() +
                                " | FPS " + eastl::to_string(static_cast<i32>(1.0 / mLastDeltaTime)));
    }

    void VisualTestApp::RebuildTaskGraph() {
        mTaskRenderGraph->Reset();
        ActiveTest()->CreateResources({
            .displayInfo = { .width = WIDTH, .height = HEIGHT },
            .shaderCompiler = *mShaderCompiler,
            .resourceManager = *mTaskResourceManager,
        });
        for (GenericTask* task : ActiveTest()->CreateTasks()) {
            if (auto* gtask = dynamic_cast<GraphicsTask*>(task); gtask) {
                mTaskRenderGraph->AddTask(gtask);
            } else if (auto* ctask = dynamic_cast<ComputeTask*>(task); ctask) {
                mTaskRenderGraph->AddTask(ctask);
            } else if (auto* ttask = dynamic_cast<TransferTask*>(task); ttask) {
                mTaskRenderGraph->AddTask(ttask);
            } else if (auto* xtask = dynamic_cast<CustomTask*>(task); xtask) {
                mTaskRenderGraph->AddTask(xtask);
            } else {
                Logger::Fatal(gSGSink, "Bad task");
            }
            mCurrentTestTasks.emplace_back(eastl::unique_ptr<GenericTask>(task));
        }
        TaskImage toComposite = ActiveTest()->GetCompositeImageTaskGraph();
        Rect2D srcRect = Rect2D::Cut({ toComposite->Info().size.x, toComposite->Info().size.y });
        Rect2D dstRect = Rect2D::Cut({ mActiveWindow->GetSize().width, mActiveWindow->GetSize().height });
        if (mRHIManager->GetAttachedRHI()->Properties().viewportConvention == RHIViewportConvention::LeftHanded_OriginTopLeft) {
            dstRect.y = dstRect.height;
            dstRect.height *= -1;
        }
        mTaskRenderGraph->AddSwapChainWrite({
            .image = ActiveTest()->GetCompositeImageTaskGraph(),
            .swapChain = mSwapChain,
            .srcRect = srcRect,
            .dstRect = dstRect,
        });
        mTaskRenderGraph->Build();

        printf("%s\n", mTaskRenderGraph->ToString().c_str());
    }

    eastl::string VisualTestApp::GetFullTestTitle() {
        return "Test " + eastl::to_string(mCurrentTest + 1) + " / " + eastl::to_string(mTests.size()) +
               " | " + ActiveTest()->Title() + " | RHI " + mRHIManager->GetAttachedRHIInfo().info.name + (bDebugLayers ? " (DEBUG LAYERS ON)" : "");
    }
} // namespace VisualTests