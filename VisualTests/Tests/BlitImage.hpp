#pragma once
#include <VisualTests/IVisualTest.hpp>


namespace VisualTests {
    class BlitImage : public IVisualTest, DeleteCopy, DeleteMove {
        eastl::string Title() const override { return "Blit Image"; }

        void CreateResources(const CreateResourceInfo& info) override;
        void ReleaseResources(const ReleaseResourceInfo& info) override;
        eastl::span<GenericTask*> CreateTasks() override;

        bool UseTaskGraph() const override { return true; }
        TaskImage GetCompositeImageTaskGraph() override { return blitImage; }
        Image GetCompositeImageRaw() override { return blitImage->Internal(); }

    private:
        TaskImage image;
        TaskColorTarget target;
        TaskImage blitImage;
        TaskShader vsh, fsh;
        TaskRasterPipeline pipeline;

        eastl::vector<GenericTask*> tasks = {};
    };
} // namespace VisualTests