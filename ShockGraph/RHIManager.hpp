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

#ifdef SHOCKGRAPH_USE_PYRO_PLATFORM
#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>

#include <PyroCommon/Core.hpp>
#include <PyroCommon/LoggerInterface.hpp>
#include <PyroPlatform/Forward.hpp>
#include <PyroRHI/Api/Forward.hpp>
#include <PyroRHI/Exports.hpp>
#include <PyroRHI/Info.hpp>
#include <PyroRHI/Shader/Forward.hpp>
#include <ShockGraph/Core.hpp>

namespace PyroshockStudios {
    inline namespace Renderer {
        class RHIManager : public ILoggerAware, DeleteCopy, DeleteMove {
        public:
            struct AttachableRHIInfo {
                RHIInfo info = {};
                IDynamicLibrary* library = {};
                PFN_CreateRHIContext fnCreateRHIContext = {};
                PFN_DestroyRHIContext fnDestroyRHIContext = {};
            };

            SHOCKGRAPH_API RHIManager();
            SHOCKGRAPH_API ~RHIManager();

            SHOCKGRAPH_API void DiscoverAvailableRHIs();

            // Returns true if the guid exists, successfully created context, and there is no currently attached RHI
            SHOCKGRAPH_API bool AttachRHI(GUID rhiGUID, const RHICreateInfo& createInfo);
            SHOCKGRAPH_API const AttachableRHIInfo& GetAttachedRHIInfo();
            SHOCKGRAPH_API RHIContext* GetAttachedRHI();
            SHOCKGRAPH_API IDevice* GetRHIDevice();

            SHOCKGRAPH_API eastl::vector<RHIInfo> QueryAvailableRHIs() const;

            SHOCKGRAPH_API void InjectLogger(ILogStream* stream) override {
                mLogStream = stream;
            }

        private:
            // detaches the current attached RHI
            bool DetachRHI();

            void ReleaseAvailableRHIs();

            eastl::vector<AttachableRHIInfo> mAvailableRHIs = {};
            AttachableRHIInfo mAttachedRHIInfo = {};
            RHIContextApiInfo mRhiApi = {};
            IDevice* mRhiDevice = nullptr;
            ILogStream* mLogStream = nullptr;
        };
    } // namespace Renderer
} // namespace PyroshockStudios
#endif