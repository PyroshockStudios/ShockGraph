#pragma once
#include <VisualTests/IVisualTest.hpp>


namespace VisualTests {
    class MSAA : public IVisualTest, DeleteCopy, DeleteMove {
        eastl::string Title() const override { return "MSAA"; }

        void CreateResources(const CreateResourceInfo& info) override;
        void ReleaseResources(const ReleaseResourceInfo& info) override;
        eastl::span<GenericTask*> CreateTasks() override;

        bool UseTaskGraph() const override { return true; }
        TaskImage GetCompositeImageTaskGraph() override { return image; }
        Image GetCompositeImageRaw() override { return image->Internal(); }

    private:
        TaskImage imageMSAA;
        TaskImage image;
        TaskColorTarget targetMSAA;
        TaskColorTarget target;
        TaskShader vsh, fsh;
        TaskRasterPipeline pipelineMSAA;
        TaskRasterPipeline pipeline;

        eastl::vector<GenericTask*> tasks = {};
    };
} // namespace VisualTests