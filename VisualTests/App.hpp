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
    class StdoutLogger : public ILogStream {
    public:
        StdoutLogger(const char* name) {
            usize len = strlen(name);
            mName = new char[len + 1];
            strncpy(mName, name, len);
            memset(mName + len, 0, sizeof(char));
        }
        virtual ~StdoutLogger() {
            delete[] mName;
        }
        virtual void Log(LogSeverity severity, const char* message) const {
            const char* sevStr = "";
            switch (severity) {
            case LogSeverity::Verbose:
                sevStr = "Verbose";
                break;
            case LogSeverity::Debug:
                sevStr = "Debug";
                break;
            case LogSeverity::Trace:
                sevStr = "Trace";
                break;
            case LogSeverity::Info:
                sevStr = "Info";
                break;
            case LogSeverity::Warn:
                sevStr = "Warn";
                break;
            case LogSeverity::Error:
                sevStr = "Error";
                break;
            case LogSeverity::Fatal:
                sevStr = "Fatal";
                break;
            default:
                break;
            }
            printf("[%s] [%s] %s\n", Name(), sevStr, message);
            if (severity == LogSeverity::Fatal) {
                abort();
            }
        }
        virtual LogSeverity MinSeverity() const {
            return LogSeverity::Trace;
        }
        virtual const char* Name() const override {
            return mName;
        }

    private:
         char* mName = nullptr;
    };

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
} // namespace VisualTests