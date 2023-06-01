#include "RenderEngine.h"
#include "RenderGraph.h"
#include "RenderGraphBuilder.h"
#include "CommandBuffer.h"

#include "Windows/Window.h"
#include "Windows/InputManger.h"

#include "GHL/DebugLayer.h"

using namespace Renderer;
using namespace Windows;

void Test_RenderGraphBuildDAG(RenderEngine& renderEngine) {
    RenderGraph& renderGraph = *renderEngine.mRenderGraph.get();

    renderGraph.AddPass(
        "Pass0",
        [=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
            builder.SetPassExecutionQueue(GHL::EGPUQueue::Graphics);

            NewTextureProperties properties{};
            properties.width = 1920u;
            properties.height = 1080u;
            properties.format = DXGI_FORMAT_R8G8B8A8_UNORM;
            // builder.NewTexture("Tex_0", properties);

        },
        [=](CommandBuffer& commandBuffer, RenderContext& context) {}
    );

    renderGraph.AddPass(
        "Pass1",
        [=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
            builder.SetPassExecutionQueue(GHL::EGPUQueue::Graphics);

            builder.ReadTexture("Tex_0", ShaderAccessFlag::PixelShader);

            NewTextureProperties properties{};
            properties.width = 1920u;
            properties.height = 1080u;
            properties.format = DXGI_FORMAT_R8G8B8A8_UNORM;
            // builder.NewTexture("Tex_1", properties);

        },
        [=](CommandBuffer& commandBuffer, RenderContext& context) {}
        );

    renderGraph.AddPass(
        "Pass2",
        [=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
            builder.SetPassExecutionQueue(GHL::EGPUQueue::Graphics);

            NewTextureProperties properties{};
            properties.width = 1920u;
            properties.height = 1080u;
            properties.format = DXGI_FORMAT_R8G8B8A8_UNORM;
            builder.ReadTexture("Tex_0", ShaderAccessFlag::PixelShader);

        },
        [=](CommandBuffer& commandBuffer, RenderContext& context) {}
        );

    renderGraph.AddPass(
        "Pass3",
        [=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
            builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

            builder.ReadTexture("Tex_0", ShaderAccessFlag::NonPixelShader);
            // builder.NewTexture("Tex_3", NewTextureProperties{});

        },
        [=](CommandBuffer& commandBuffer, RenderContext& context) {}
        );

    renderGraph.AddPass(
        "Pass4",
        [=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
            builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

            builder.ReadTexture("Tex_0", ShaderAccessFlag::NonPixelShader);
            builder.ReadTexture("Tex_1", ShaderAccessFlag::NonPixelShader);
            builder.ReadTexture("Tex_3", ShaderAccessFlag::NonPixelShader);

        },
        [=](CommandBuffer& commandBuffer, RenderContext& context) {}
        );

    renderGraph.AddPass(
        "Pass5",
        [=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
            builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

            builder.ReadTexture("Tex_0", ShaderAccessFlag::NonPixelShader);

        },
        [=](CommandBuffer& commandBuffer, RenderContext& context) {}
        );

    renderGraph.AddPass(
        "Pass6",
        [=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
            builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

            builder.ReadTexture("Tex_1", ShaderAccessFlag::NonPixelShader);

        },
        [=](CommandBuffer& commandBuffer, RenderContext& context) {}
        );

    renderGraph.Build();
}

int WINAPI main(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    GHL::EnableDebugLayer();

    WindowSetting setting{};
    setting.fullscreen = false;
    Window window{ setting };
    InputManger inputManger{ &window };

    RenderEngine renderEngine(window.GetHWND(), setting.width, setting.height);
    // Test_RenderGraphBuildDAG(renderEngine);

    bool done = false;
    while (!done) {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                done = true;
            }
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
        if (done) {
            break;
        }
        renderEngine.Render();
    }

    return 0;
}