#include "RenderGraph.h"
#include "RenderGraphBuilder.h"

using namespace Renderer;

void Test_RenderGraphBuildDAG() {
    RenderGraph renderGraph(nullptr);

    renderGraph.AddPass(
        "Pass0",
        [=](RenderGraphBuilder& builder) {
            builder.SetPassExecutionQueue(PassExecutionQueue::General);

            builder.WriteTexture("Tex_0");

        },
        [=]() {}
    );

    renderGraph.AddPass(
        "Pass1",
        [=](RenderGraphBuilder& builder) {
            builder.SetPassExecutionQueue(PassExecutionQueue::General);

            builder.ReadTexture("Tex_0");
            builder.WriteTexture("Tex_1");

        },
        [=]() {}
        );

    renderGraph.AddPass(
        "Pass2",
        [=](RenderGraphBuilder& builder) {
            builder.SetPassExecutionQueue(PassExecutionQueue::General);

            builder.ReadTexture("Tex_0");
            builder.ReadTexture("Tex_1");

        },
        [=]() {}
        );

    renderGraph.AddPass(
        "Pass3",
        [=](RenderGraphBuilder& builder) {
            builder.SetPassExecutionQueue(PassExecutionQueue::Compute);

            builder.ReadTexture("Tex_0");
            builder.WriteTexture("Tex_3");

        },
        [=]() {}
        );

    renderGraph.AddPass(
        "Pass4",
        [=](RenderGraphBuilder& builder) {
            builder.SetPassExecutionQueue(PassExecutionQueue::Compute);

            builder.ReadTexture("Tex_0");
            builder.ReadTexture("Tex_1");
            builder.ReadTexture("Tex_3");

        },
        [=]() {}
        );

    renderGraph.AddPass(
        "Pass5",
        [=](RenderGraphBuilder& builder) {
            builder.SetPassExecutionQueue(PassExecutionQueue::Compute);

            builder.ReadTexture("Tex_0");

        },
        [=]() {}
        );

    renderGraph.AddPass(
        "Pass6",
        [=](RenderGraphBuilder& builder) {
            builder.SetPassExecutionQueue(PassExecutionQueue::Compute);

            builder.ReadTexture("Tex_1");

        },
        [=]() {}
        );

    renderGraph.Build();
}

int WINAPI main(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    Test_RenderGraphBuildDAG();

    int64_t i = 32;

    return 0;
}