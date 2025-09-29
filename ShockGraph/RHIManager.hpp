#pragma once

#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>

#include <PyroCommon/Core.hpp>
#include <PyroPlatform/Forward.hpp>
#include <PyroRHI/Api/Forward.hpp>
#include <PyroRHI/Exports.hpp>
#include <PyroRHI/Info.hpp>
#include <PyroRHI/Shader/Forward.hpp>
#include <ShockGraph/Core.hpp>

namespace PyroshockStudios {
    inline namespace Renderer {
        class RHIManager : DeleteCopy, DeleteMove {
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

        private:
            // detaches the current attached RHI
            bool DetachRHI();

            void ReleaseAvailableRHIs();

            eastl::vector<AttachableRHIInfo> mAvailableRHIs = {};
            AttachableRHIInfo mAttachedRHIInfo = {};
            RHIContextApiInfo mRhiApi = {};
            IDevice* mRhiDevice = nullptr;
        };
    }
}