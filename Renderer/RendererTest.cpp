#include "RenderGraph.h"
#include "RenderGraphBuilder.h"

using namespace Renderer;

void Test_RenderGraphBuildDAG() {
    RenderGraph renderGraph(nullptr);

    renderGraph.AddPass(
        "Pass1",
        [=](RenderGraphBuilder& builder) {
            builder.SetPassExecutionQueue(PassExecutionQueue::General);

            builder.WriteTexture("Tex_1");
            builder.WriteTexture("Tex_2");
            builder.WriteTexture("Tex_3");
            builder.WriteTexture("Tex_4");

        },
        [=]() {}
    );

    renderGraph.AddPass(
        "Pass2",
        [=](RenderGraphBuilder& builder) {
            builder.SetPassExecutionQueue(PassExecutionQueue::Compute);



        },
        [=]() {}
        );

}

int WINAPI main(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    Test_RenderGraphBuildDAG();

    int64_t i = 32;

    return 0;
}