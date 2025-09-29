#include "RHIManager.hpp"
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
        RHIManager::RHIManager() {
        }
        RHIManager::~RHIManager() {
            DetachRHI();
            ReleaseAvailableRHIs();
        }

        void RHIManager::DiscoverAvailableRHIs() {
            ReleaseAvailableRHIs();
            eastl::string rhiDirectory = PlatformFactory::Get<IFileSystem>()->GetExecutableDirectory() + "/RHI";

            Logger::Trace("Searching for RHIs inside " + rhiDirectory);
            std::filesystem::directory_iterator iterator;
            try {
                iterator = std::filesystem::directory_iterator(rhiDirectory.c_str());
            } catch (std::exception ex) {
                Logger::Error("Failed to query RHIs inside directory \"" + rhiDirectory + "\". Exception thrown, reason: \"" + eastl::string(ex.what()) + "\"");
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
                        Logger::Error("Failed to query RHI from path \"" + rhiPath + "\"");
                        continue;
                    }

                    AttachableRHIInfo attachable{
                        .library = lib,
                        .fnCreateRHIContext = lib->GetAddress<PFN_CreateRHIContext>("CreateRHIContext"),
                        .fnDestroyRHIContext = lib->GetAddress<PFN_DestroyRHIContext>("DestroyRHIContext")
                    };
                    PFN_GetCustomRHIInfo fnGetInfo = lib->GetAddress<PFN_GetCustomRHIInfo>("GetCustomRHIInfo");

                    if (!fnGetInfo || !attachable.fnCreateRHIContext || !attachable.fnDestroyRHIContext) {
                        Logger::Error("RHI \"" + rhiPath + "\" is missing exported functions! PFN_CreateRHIContext/PFN_DestroyRHIContext/PFN_GetCustomRHIInfo may be missing. Ignoring RHI...");
                        PlatformFactory::Get<ILibraryLoader>()->Unload(lib);
                        continue;
                    }

                    fnGetInfo(&attachable.info);

                    if (!attachable.info.guid.Valid()) {
                        Logger::Error("RHI \"{}\" has a bad GUID. Ignoring RHI...", rhiPath);
                        PlatformFactory::Get<ILibraryLoader>()->Unload(lib);
                        continue;
                    }

                    Logger::Trace("Found RHI " + attachable.info.guid.ToString() + " '" + eastl::string(attachable.info.name) + "' by '" + eastl::string(attachable.info.author) + "'");
                    mAvailableRHIs.emplace_back(attachable);
                }
            }
        }

        bool RHIManager::AttachRHI(GUID rhiGUID, const RHICreateInfo& createInfo) {
            if (mAttachedRHIInfo.library != nullptr) {
                Logger::Error("RHI " + eastl::string(mAttachedRHIInfo.info.name) + " is currently attached! Application must be restarted to use a different RHI!.");
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
                Logger::Error("Failed to attach RHI " + eastl::string(mAttachedRHIInfo.info.name) + "! RHIContext creation failed!");
                return false;
            }
            mRhiDevice = mRhiApi.loadedContext->CreateDevice();

            return true;
        }
        const RHIManager::AttachableRHIInfo& RHIManager::GetAttachedRHIInfo() {
            return mAttachedRHIInfo;
        }
        RHIContext* RHIManager::GetAttachedRHI() {
            return mRhiApi.loadedContext;
        }
        IDevice* RHIManager::GetRHIDevice() {
            return mRhiDevice;
        }

        eastl::vector<RHIInfo> RHIManager::QueryAvailableRHIs() const {
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
    }
}