#pragma once
#include <VisualTests/IVisualTest.hpp>


namespace VisualTests {
    class UniformBuffer : public IVisualTest, DeleteCopy, DeleteMove {
        eastl::string Title() const override { return "Uniform Buffer"; }

        void CreateResources(const CreateResourceInfo& info) override;
        void ReleaseResources(const ReleaseResourceInfo& info) override;
        eastl::span<GenericTask*> CreateTasks() override;

        bool UseTaskGraph() const override { return true; }
        TaskImage GetCompositeImageTaskGraph() override { return image; }
        Image GetCompositeImageRaw() override { return image->Internal(); }

    private:
        TaskImage image;
        TaskBuffer ubo;
        TaskColorTarget target;
        TaskShader vsh, fsh;
        TaskRasterPipeline pipeline;

        eastl::vector<GenericTask*> tasks = {};
    };
} // namespace VisualTests