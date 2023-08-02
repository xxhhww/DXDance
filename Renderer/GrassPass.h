#pragma once
#include "Renderer/RenderGraph.h"
#include "Renderer/ResourceAllocator.h"
#include "Renderer/Model.h"

namespace Renderer {

	class RenderEngine;

	class GrassPass {
	public:
		struct GrassGeneratorPassData {
			uint32_t grassBladeBufferIndex0;	// Grass LOD0
			uint32_t grassBladeBufferIndex1;	// Grass LOD1
			uint32_t terrainHeightMapIndex;
			uint32_t terrainNormalMapIndex;

			Math::Vector2 worldMeterSize{ 5120u, 5120u };	// 世界在XZ轴方向的大小(米)
			uint32_t heightScale{ 4096u };					// 世界的Y轴缩放
			uint32_t grassBladeSizePerAxis;

			uint32_t useFrustumCull{ 1u };
			uint32_t useDistanceCull{ 1u };
			float    distanceCullStartDist{ 5.0f };
			float    distanceCullEndDist{ 55.0f };

			float    distanceCullMinimumGrassAmount{ 0.1f };
			float    jitterStrength{ 5u };
		};

		struct GrassRendererPassData {
			uint32_t grassBladeBufferIndex0;
			uint32_t grassBladeBufferIndex1;
			uint32_t grassMeshIndicesBufferIndex;
			uint32_t grassMeshVerticesBufferIndex;
			
			float p1Flexibility{ 1.0f };
			float p2Flexibility{ 1.0f };
			float waveAmplitude{ 70.0f };
			float waveSpeed{ 100.0f };

			float wavePower{ 2.0f };
			float sinOffsetRange{ -1.0f };
			float pushTipOscillationForward{ 1.0f };
			float widthTaperAmount{ 0.37f };
		};

		struct GrassBlade {

			Math::Vector3 position;
			Math::Vector2 facing;

			float windStrength;

			float hash;

			uint32_t type;

			float height;
			float width;
			float tilt;
			float bend;

			float sideCurve;
		};


		struct ClumpParametersStruct {
			float pullToCentre;
			float pointInSameDirection;
			float baseHeight;
			float heightRandom;
			float baseWidth;
			float widthRandom;
			float baseTilt;
			float tiltRandom;
			float baseBend;
			float bendRandom;
		};

		GrassGeneratorPassData grassGeneratorPassData;
		GrassRendererPassData  grassRendererPassData;

		inline static uint32_t smThreadSizeInGroup = 8u;
		inline static uint32_t smGrassBladeSizePerAxis = 4096u;		// 每个轴的安置点的个数
		inline static uint32_t smMaxGrassBladeSize = 2048u * 2048u;	// 安置点的个数的最大值															// 安置点数组

		std::vector<Vertex>   grassVertices;
		std::vector<uint32_t> grassIndices;
	public:
		void AddPass(RenderGraph& renderGraph);

		void InitializePass(RenderEngine* renderEngine);

	private:
	};

}