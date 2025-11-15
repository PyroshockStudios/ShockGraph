// MIT License
//
// Copyright (c) 2025 Pyroshock Studios
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#define PYRO_IMPLEMENT_NEW_OPERATOR


#include <PyroCommon/MemoryOverload.hpp>

#include <cstdlib>
#include <stdexcept>

#include "Tests/AlphaBlending.hpp"
#include "Tests/AlphaToCoverage.hpp"
#include "Tests/BlitImage.hpp"
#include "Tests/ComputeUAV.hpp"
#include "Tests/RayQueryCompute.hpp"
#include "Tests/RayQueryPixel.hpp"
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


/**
* CONTROLS: 
* Next RHI: +
* Prev RHI: -
* Next Test: >
* Prev Test: <
* Print Task Timings: P
* Reload Test: R
*/

int main(i32 argc, char** argv) {
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
    app->RegisterTest<VisualTests::RayQueryPixel>();
    app->RegisterTest<VisualTests::RayQueryCompute>();

    app->Run();

    delete app;

    return EXIT_SUCCESS;
}