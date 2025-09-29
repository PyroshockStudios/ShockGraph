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

#include "RHIManager.hpp"

#ifdef SHOCKGRAPH_USE_PYRO_PLATFORM
#include <PyroPlatform/Factory.hpp>
#include <PyroPlatform/File/IDynamicLibrary.hpp>
#include <PyroPlatform/File/IFileSystem.hpp>
#include <PyroPlatform/File/ILibraryLoader.hpp>
#include <filesystem>

#include <PyroCommon/Logger.hpp>
#include <PyroRHI/Api/IDevice.hpp>
#include <PyroRHI/Context.hpp>
#include <PyroRHI/Shader/IShaderFeatureSet.hpp>

#include <libassert/assert.hpp>

namespace PyroshockStudios {
    inline namespace Renderer {
        SHOCKGRAPH_API RHIManager::RHIManager() {
        }
        SHOCKGRAPH_API RHIManager::~RHIManager() {
            DetachRHI();
            ReleaseAvailableRHIs();
        }

        SHOCKGRAPH_API void RHIManager::DiscoverAvailableRHIs() {
            ReleaseAvailableRHIs();
            eastl::string rhiDirectory = PlatformFactory::Get<IFileSystem>()->GetExecutableDirectory() + "/RHI";

            Logger::Trace(mLogStream, "Searching for RHIs inside " + rhiDirectory);
            std::filesystem::directory_iterator iterator;
            try {
                iterator = std::filesystem::directory_iterator(rhiDirectory.c_str());
            } catch (std::exception ex) {
                Logger::Error(mLogStream, "Failed to query RHIs inside directory \"" + rhiDirectory + "\". Exception thrown, reason: \"" + eastl::string(ex.what()) + "\"");
                return;
            }

            static const eastl::vector<std::filesystem::path> dllFileExtensions = {
                ".dll", ".so", ".dylib", ".framework"
            };

            for (const auto& path : iterator) {
                if (path.exists() && eastl::find(dllFileExtensions.begin(), dllFileExtensions.end(), path.path().extension()) != dllFileExtensions.end()) {
                    eastl::string rhiPath = path.path().string().c_str();
                    IDynamicLibrary* lib = PlatformFactory::Get<ILibraryLoader>()->Load(rhiPath);
                    if (!lib) {
                        // TODO
                        Logger::Error(mLogStream, "Failed to query RHI from path \"" + rhiPath + "\"");
                        continue;
                    }

                    AttachableRHIInfo attachable{
                        .library = lib,
                        .fnCreateRHIContext = lib->GetAddress<PFN_CreateRHIContext>("CreateRHIContext"),
                        .fnDestroyRHIContext = lib->GetAddress<PFN_DestroyRHIContext>("DestroyRHIContext")
                    };
                    PFN_GetCustomRHIInfo fnGetInfo = lib->GetAddress<PFN_GetCustomRHIInfo>("GetCustomRHIInfo");

                    if (!fnGetInfo || !attachable.fnCreateRHIContext || !attachable.fnDestroyRHIContext) {
                        Logger::Error(mLogStream, "RHI \"" + rhiPath + "\" is missing exported functions! PFN_CreateRHIContext/PFN_DestroyRHIContext/PFN_GetCustomRHIInfo may be missing. Ignoring RHI...");
                        PlatformFactory::Get<ILibraryLoader>()->Unload(lib);
                        continue;
                    }

                    fnGetInfo(&attachable.info);

                    if (!attachable.info.guid.Valid()) {
                        Logger::Error(mLogStream, "RHI \"{}\" has a bad GUID. Ignoring RHI...", rhiPath);
                        PlatformFactory::Get<ILibraryLoader>()->Unload(lib);
                        continue;
                    }

                    Logger::Trace(mLogStream, "Found RHI " + attachable.info.guid.ToString() + " '" + eastl::string(attachable.info.name) + "' by '" + eastl::string(attachable.info.author) + "'");
                    mAvailableRHIs.emplace_back(attachable);
                }
            }
        }

        SHOCKGRAPH_API bool RHIManager::AttachRHI(GUID rhiGUID, const RHICreateInfo& createInfo) {
            if (mAttachedRHIInfo.library != nullptr) {
                Logger::Error(mLogStream, "RHI " + eastl::string(mAttachedRHIInfo.info.name) + " is currently attached! Application must be restarted to use a different RHI!.");
                return false;
            }
            for (const auto& rhi : mAvailableRHIs) {
                if (rhi.info.guid == rhiGUID) {
                    mAttachedRHIInfo = rhi;
                    break;
                }
            }

            mRhiApi = {};
            mAttachedRHIInfo.fnCreateRHIContext(&createInfo, &mRhiApi);
            if (mRhiApi.loadedContext == nullptr) {
                Logger::Error(mLogStream, "Failed to attach RHI " + eastl::string(mAttachedRHIInfo.info.name) + "! RHIContext creation failed!");
                return false;
            }
            mRhiDevice = mRhiApi.loadedContext->CreateDevice();

            return true;
        }
        SHOCKGRAPH_API const RHIManager::AttachableRHIInfo& RHIManager::GetAttachedRHIInfo() {
            return mAttachedRHIInfo;
        }
        SHOCKGRAPH_API RHIContext* RHIManager::GetAttachedRHI() {
            return mRhiApi.loadedContext;
        }
        SHOCKGRAPH_API IDevice* RHIManager::GetRHIDevice() {
            return mRhiDevice;
        }

        SHOCKGRAPH_API eastl::vector<RHIInfo> RHIManager::QueryAvailableRHIs() const {
            eastl::vector<RHIInfo> infos{};
            infos.reserve(mAvailableRHIs.size());
            for (const auto& attachable : mAvailableRHIs) {
                infos.emplace_back(attachable.info);
            }
            return infos;
        }

        bool RHIManager::DetachRHI() {
            if (mRhiApi.loadedContext) {
                ASSERT(mAttachedRHIInfo.library != nullptr, "The RHI library must persist until the context has been safely destroyed!");
                mAttachedRHIInfo.fnDestroyRHIContext(&mRhiApi);
                mRhiApi = {};
                mAttachedRHIInfo = {};
                return true;
            }
            return false;
        }

        void RHIManager::ReleaseAvailableRHIs() {
            for (auto& attachable : mAvailableRHIs) {
                PlatformFactory::Get<ILibraryLoader>()->Unload(attachable.library);
            }
            mAvailableRHIs.clear();
        }
    } // namespace Renderer
} // namespace PyroshockStudios
#endif