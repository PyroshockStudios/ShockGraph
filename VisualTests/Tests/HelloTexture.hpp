#pragma once
#include <VisualTests/IVisualTest.hpp>


namespace VisualTests {
    class HelloTexture : public IVisualTest, DeleteCopy, DeleteMove {
        eastl::string Title() const override { return "Hello Texture"; }

        void CreateResources(const CreateResourceInfo& info) override;
        void ReleaseResources(const ReleaseResourceInfo& info) override;
        eastl::span<GenericTask*> CreateTasks() override;

        bool UseTaskGraph() const override { return true; }
        TaskImage GetCompositeImageTaskGraph() override { return image; }
        Image GetCompositeImageRaw() override { return image->Internal(); }

    private:
        TaskImage image;
        TaskImage texture;
        ShaderResourceId textureView;
        SamplerId sampler;
        TaskColorTarget target;
        TaskShader vsh, fsh;
        TaskRasterPipeline pipeline;

        eastl::vector<GenericTask*> tasks = {};
    };
} // namespace VisualTests