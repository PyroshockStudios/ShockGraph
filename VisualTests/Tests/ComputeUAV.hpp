#pragma once
#include <VisualTests/IVisualTest.hpp>


namespace VisualTests {
    class ComputeUAV : public IVisualTest, DeleteCopy, DeleteMove {
        eastl::string Title() const override { return "Compute-UAV"; }

        void CreateResources(const CreateResourceInfo& info) override;
        void ReleaseResources(const ReleaseResourceInfo& info) override;
        eastl::span<GenericTask*> CreateTasks() override;

        bool UseTaskGraph() const override { return true; }
        TaskImage GetCompositeImageTaskGraph() override { return image; }
        Image GetCompositeImageRaw() override { return image->Internal(); }

    private:
        TaskImage image;
        TaskImage depth;
        TaskBuffer vboUav;
        TaskBuffer idxUav;
        UnorderedAccessId vboUaView;
        UnorderedAccessId idxUaView;
        TaskColorTarget target;
        TaskDepthStencilTarget depthTarget;
        TaskShader vsh, fsh, csh;
        TaskRasterPipeline renderVertices;
        TaskComputePipeline generateVertices;

        eastl::vector<GenericTask*> tasks = {};
    };
} // namespace VisualTests