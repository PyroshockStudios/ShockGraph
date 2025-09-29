#include "SpecialisationConstants.hpp"

#include "PyroRHI/Api/IDevice.hpp"

namespace VisualTests {
    void SpecialisationConstants::CreateResources(const CreateResourceInfo& info) {
        image = info.resourceManager.CreatePersistentImage({
            .format = Format::RGBA8Unorm,
            .size = { info.displayInfo.width, info.displayInfo.height },
            .usage = ImageUsageFlagBits::RENDER_TARGET | ImageUsageFlagBits::TRANSFER_SRC | ImageUsageFlagBits::BLIT_SRC,
            .name = "Specialisation Constants Render Image",
        });
        target = info.resourceManager.CreateColorTarget({
            .image = image,
            .name = "Specialisation Constants RT",
        });
        vsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/SpecialisationConstants.slang",
            { .stage = ShaderStage::Vertex, .entryPoint = "vertexMain", .name = "Specialisation Constants Vsh" });
        fsh = info.shaderCompiler.CompileShaderFromFile("resources/VisualTests/Shaders/SpecialisationConstants.slang",
            { .stage = ShaderStage::Fragment, .entryPoint = "fragmentMain", .name = "Specialisation Constants Fsh" });
        eastl::array<SpecializationConstantInfo, 2> specialisationConstants = {};
        specialisationConstants[0].location = 0;
        specialisationConstants[0].data = 0.25f;
        specialisationConstants[1].location = 1;
        specialisationConstants[1].data = -0.25f;
        pipeline0 = info.resourceManager.CreateRasterPipeline(
            {
                .colorTargetStates = { { .format = image->Info().format } },
                .name = "Raster Pipeline 0",
            },
            {
                .vertexShaderInfo = {
                    TaskShaderInfo{
                        .program = vsh,
                        .specializationConstants = specialisationConstants,
                    } },
                .fragmentShaderInfo = { TaskShaderInfo{ .program = fsh } },
            });

        specialisationConstants[0].location = 0;
        specialisationConstants[0].data = 0.50f;
        specialisationConstants[1].location = 1;
        specialisationConstants[1].data = 0.25f;
        pipeline1 = info.resourceManager.CreateRasterPipeline(
            {
                .colorTargetStates = { { .format = image->Info().format } },
                .name = "Raster Pipeline 1",
            },
            {
                .vertexShaderInfo = {
                    TaskShaderInfo{
                        .program = vsh,
                        .specializationConstants = specialisationConstants,
                    } },
                .fragmentShaderInfo = { TaskShaderInfo{ .program = fsh } },
            });
    }
    void SpecialisationConstants::ReleaseResources(const ReleaseResourceInfo& info) {
        image = {};
        target = {};
        vsh, fsh = {};
        pipeline0 = {};
        pipeline1 = {};
    }
    eastl::span<GenericTask*> SpecialisationConstants::CreateTasks() {
        tasks = {
            new GraphicsCallbackTask(
                { .name = "Specialisation Constants", .color = LabelColor::GREEN },
                [this](GraphicsTask& task) {
                    task.BindColorTarget({
                        .target = target,
                        .clear = { { 0.0f, 0.0f, 0.0f, 1.0f } },
                    });
                },
                [this](TaskCommandList& commands) {
                    commands.SetRasterPipeline(pipeline0);
                    commands.Draw({ .vertexCount = 3 });
                    commands.SetRasterPipeline(pipeline1);
                    commands.Draw({ .vertexCount = 3 });
                })
        };
        return tasks;
    }
} // namespace VisualTests