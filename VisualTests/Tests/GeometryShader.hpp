#pragma once
#include <VisualTests/IVisualTest.hpp>


namespace VisualTests {
    class GeometryShader : public IVisualTest, DeleteCopy, DeleteMove {
        eastl::string Title() const override { return "Geometry Shader"; }

        void CreateResources(const CreateResourceInfo& info) override;
        void ReleaseResources(const ReleaseResourceInfo& info) override;
        eastl::span<GenericTask*> CreateTasks() override;

        bool UseTaskGraph() const override { return true; }
        TaskImage GetCompositeImageTaskGraph() override { return image; }
        Image GetCompositeImageRaw() override { return image->Internal(); }

    private:
        TaskImage image;
        TaskColorTarget target;
        TaskShader vsh, gsh, fsh;
        TaskRasterPipeline pipeline;

        eastl::vector<GenericTask*> tasks = {};
    };
} // namespace VisualTests