#include "RenderGraph.h"
#include "RenderGraphBuilder.h"

using namespace Renderer;

void Test_RenderGraphBuildDAG() {
    RenderGraph renderGraph(nullptr, nullptr);

    renderGraph.AddPass(
        "Pass0",
        [=](RenderGraphBuilder& builder) {
            builder.SetPassExecutionQueue(PassExecutionQueue::General);

            builder.NewTexture("Tex_0", NewTextureProperties{});

        },
        [=]() {}
    );

    renderGraph.AddPass(
        "Pass1",
        [=](RenderGraphBuilder& builder) {
            builder.SetPassExecutionQueue(PassExecutionQueue::General);

            builder.ReadTexture("Tex_0", ShaderAccessFlag::PixelShader);
            builder.NewTexture("Tex_1", NewTextureProperties{});

        },
        [=]() {}
        );

    renderGraph.AddPass(
        "Pass2",
        [=](RenderGraphBuilder& builder) {
            builder.SetPassExecutionQueue(PassExecutionQueue::General);

            builder.ReadTexture("Tex_0", ShaderAccessFlag::PixelShader);
            builder.ReadTexture("Tex_1", ShaderAccessFlag::PixelShader);

        },
        [=]() {}
        );

    renderGraph.AddPass(
        "Pass3",
        [=](RenderGraphBuilder& builder) {
            builder.SetPassExecutionQueue(PassExecutionQueue::Compute);

            builder.ReadTexture("Tex_0", ShaderAccessFlag::NonPixelShader);
            builder.NewTexture("Tex_3", NewTextureProperties{});

        },
        [=]() {}
        );

    renderGraph.AddPass(
        "Pass4",
        [=](RenderGraphBuilder& builder) {
            builder.SetPassExecutionQueue(PassExecutionQueue::Compute);

            builder.ReadTexture("Tex_0", ShaderAccessFlag::NonPixelShader);
            builder.ReadTexture("Tex_1", ShaderAccessFlag::NonPixelShader);
            builder.ReadTexture("Tex_3", ShaderAccessFlag::NonPixelShader);

        },
        [=]() {}
        );

    renderGraph.AddPass(
        "Pass5",
        [=](RenderGraphBuilder& builder) {
            builder.SetPassExecutionQueue(PassExecutionQueue::Compute);

            builder.ReadTexture("Tex_0", ShaderAccessFlag::NonPixelShader);

        },
        [=]() {}
        );

    renderGraph.AddPass(
        "Pass6",
        [=](RenderGraphBuilder& builder) {
            builder.SetPassExecutionQueue(PassExecutionQueue::Compute);

            builder.ReadTexture("Tex_1", ShaderAccessFlag::NonPixelShader);

        },
        [=]() {}
        );

    renderGraph.Build();

    int i = 32;
}

int WINAPI main(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    Test_RenderGraphBuildDAG();

    int64_t i = 32;

    return 0;
}