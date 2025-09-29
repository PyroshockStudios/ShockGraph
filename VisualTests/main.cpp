#define PYRO_IMPLEMENT_NEW_OPERATOR


#include <PyroCommon/MemoryOverload.hpp>

#include <cstdlib>
#include <stdexcept>

#include "Tests/AlphaBlending.hpp"
#include "Tests/AlphaToCoverage.hpp"
#include "Tests/BlitImage.hpp"
#include "Tests/ComputeUAV.hpp"
#include "Tests/DrawIndirect.hpp"
#include "Tests/GeometryShader.hpp"
#include "Tests/HelloTexture.hpp"
#include "Tests/HelloTriangle.hpp"
#include "Tests/IndexBuffer.hpp"
#include "Tests/InstanceBuffer.hpp"
#include "Tests/MSAA.hpp"
#include "Tests/PushConstants.hpp"
#include "Tests/SpecialisationConstants.hpp"
#include "Tests/TesselationShader.hpp"
#include "Tests/UniformBuffer.hpp"
#include "Tests/UpdateBuffer.hpp"
#include "Tests/VertexBuffer.hpp"
#include "Tests/Wireframe.hpp"
#include "Tests/WireframeSmooth.hpp"


#include <PyroCommon/Logger.hpp>
#include <VisualTests/App.hpp>
using namespace PyroshockStudios;
using namespace PyroshockStudios::Types;
using namespace VisualTests;
int main(i32 argc, char** argv) {
    Logger::Info("Starting Visual Tests...");

    VisualTestApp* app = new VisualTestApp();
    app->RegisterTest<VisualTests::HelloTriangle>();
    app->RegisterTest<VisualTests::PushConstants>();
    app->RegisterTest<VisualTests::UniformBuffer>();
    app->RegisterTest<VisualTests::VertexBuffer>();
    app->RegisterTest<VisualTests::IndexBuffer>();
    app->RegisterTest<VisualTests::DrawIndirect>();
    app->RegisterTest<VisualTests::InstanceBuffer>();
    app->RegisterTest<VisualTests::SpecialisationConstants>();
    app->RegisterTest<VisualTests::HelloTexture>();
    app->RegisterTest<VisualTests::AlphaBlending>();
    app->RegisterTest<VisualTests::MSAA>();
    app->RegisterTest<VisualTests::AlphaToCoverage>();
    app->RegisterTest<VisualTests::Wireframe>();
    app->RegisterTest<VisualTests::WireframeSmooth>();
    app->RegisterTest<VisualTests::GeometryShader>();
    app->RegisterTest<VisualTests::TesselationShader>();
    app->RegisterTest<VisualTests::ComputeUAV>();
    app->RegisterTest<VisualTests::BlitImage>();
    app->RegisterTest<VisualTests::UpdateBuffer>();

    app->Run();

    delete app;

    return EXIT_SUCCESS;
}