#pragma once
#include <VisualTests/IVisualTest.hpp>


namespace VisualTests {
    class SpecialisationConstants : public IVisualTest, DeleteCopy, DeleteMove {
        eastl::string Title() const override { return "Specialisation Constants"; }

        void CreateResources(const CreateResourceInfo& info) override;
        void ReleaseResources(const ReleaseResourceInfo& info) override;
        eastl::span<GenericTask*> CreateTasks() override;

        bool UseTaskGraph() const override { return true; }
        TaskImage GetCompositeImageTaskGraph() override { return image; }
        Image GetCompositeImageRaw() override { return image->Internal(); }

    private:
        TaskImage image;
        TaskColorTarget target;
        TaskShader vsh, fsh;
        TaskRasterPipeline pipeline0;
        TaskRasterPipeline pipeline1;

        eastl::vector<GenericTask*> tasks = {};
    };
} // namespace VisualTests