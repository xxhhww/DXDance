#include "Renderer/VegetationSystem.h"
#include "Renderer/RenderEngine.h"
#include "Renderer/RenderGraphBuilder.h"

namespace Renderer {

	void VegetationSystem::Initialize(RenderEngine* renderEngine) {

	}

	/*
	* �߼����£��������µ�GrassCluster���룬�ɵ�GrassCluster����
	*/
	void VegetationSystem::Update() {

	}

	void VegetationSystem::AddPass(RenderGraph& renderGraph) {

		/*
		* ���ɲݵ�
		*/
		renderGraph.AddPass(
			"GenerateGrassBlade",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {

			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {

			});

		/*
		* �ݵ��޳�
		*/
		renderGraph.AddPass(
			"CullGrassBlade",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {

			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {

			});

		/*
		* �ݵ���Ⱦ
		*/
		renderGraph.AddPass(
			"RenderGrassBlade",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {

			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {

			});

	}

	const Math::Vector2& VegetationSystem::GetFixedPosition(const Math::Vector2& currPosition, const Math::Vector2& gridSize) {
		return Math::Vector2{
			std::floor(currPosition.x / gridSize.x + 0.5f)* gridSize.x,
			std::floor(currPosition.y / gridSize.y + 0.5f)* gridSize.y
		};
	}

}