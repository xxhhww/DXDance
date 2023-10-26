#pragma once
#include "Renderer/VegetationPhysicalDataCache.h"
#include "Renderer/VegetationVirtualTable.h"

namespace Renderer {

	/*
	* 草的顶点属性(用于在着色器重建草的顶点坐标)
	*/
	struct GrassVertexAttribute {
	public:
		Math::Vector2 uv0;	// 顶点UV
		float t;			// 描述当前顶点沿着叶脉从草的根部移动的距离
		float side;			// 描述叶片边长(使用时需要恢复到 [-1, 1]的区域)
	};

	struct GrassBlade {
	public:
		Math::Vector3 position;
		Math::Vector2 facing;

		float    hash;
		float    height;
		float    width;
		float    tilt;		// 描述草叶的倾斜状态
		float    bend;		// 控制草叶的弯曲(其实就是控制贝塞尔样条曲线)
		float    sideCurve;	// 控制草叶的边的弯曲
	};

	struct GpuGrassCluster {
	public:
		Math::Vector4 grassClusterRect;
		uint32_t grassBladeBufferIndex;

	public:
		inline GpuGrassCluster(const Math::Vector4& _GrassClusterRect, uint32_t _GrassBladeBufferIndex)
		: grassClusterRect(_GrassClusterRect)
		, grassBladeBufferIndex(_GrassBladeBufferIndex) {}

		~GpuGrassCluster() = default;
	};

	struct ClumpParameter {
	public:
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

	class VegetationSystem {
	public:
		struct GenerateClumpMapPassData {
		public:
			Math::Vector2 clumpMapSize{ 512.0f, 512.0f };
			uint32_t      numClumps{ 3u };
			uint32_t      clumpMapIndex;
		};

		struct BakeGrassBladePassData {
		public:
			Math::Vector2 terrainWorldMeterSize;
			uint32_t      terrainHeightMapIndex;
			float         heightScale;

			uint32_t      clumpMapIndex;
			float         clumpMapScale{ 0.1f };
			uint32_t      clumpParameterBufferIndex;
			uint32_t      clumpParameterNums;

			uint32_t      needBakedGrassClusterListIndex;
			uint32_t      grassLayerMaskIndex;
			uint32_t      bakedGrassBladeListIndex;
			uint32_t      grassResolution;

			float  jitterStrength{ 5.0f };
			float  pad1;
			float  pad2;
			float  pad3;
		};

		struct CullGrassBladePassData {
		public:
			uint32_t visibleGrassClusterListIndex;			// 可见的GrassCluster列表索引
			uint32_t bakedGrassBladeListIndex;				// 烘焙的GrassBlade列表
			uint32_t visibleLOD0GrassBladeIndexListIndex;	// 可见的GrassBlade索引
			uint32_t visibleLOD1GrassBladeIndexListIndex;	// LOD1

			float grassResolution;
			float distanceCullStartDist{ 30.0f };
			float distanceCullEndDist{ 60.0f };
			float distanceCullMinimumGrassAmount{ 0.2f };
		};

		struct RenderGrassBladePassData {
		public:
			uint32_t bakedGrassBladeListIndex;				// 烘焙的GrassBlade列表
			uint32_t visibleGrassBladeIndexListIndex;		// 可见的GrassBlade索引
			uint32_t grassAlbedoMapIndex;
			uint32_t grassNormalMapIndex;

			uint32_t currLODIndex;
			uint32_t grassVertexBufferIndex;
			uint32_t grassIndexBufferIndex;
			float pad1;

			float p1Flexibility{ 1.0f };
			float p2Flexibility{ 1.0f };
			float waveAmplitude{ 70.0f };
			float waveSpeed{ 100.0f };

			float wavePower{ 2.0f };
			float sinOffsetRange{ -1.0f };
			float pushTipOscillationForward{ 1.0f };
			float widthTaperAmount{ 0.37f };
		};

	public:
		void Initialize(RenderEngine* renderEngine);

		/*
		* 逻辑更新，可能有新的GrassCluster流入，旧的GrassCluster流出，由RenderEngine的Update函数调用
		*/
		void Update(const Math::Vector2& cameraPosition);

		void AddPass(RenderGraph& renderGraph);

	private:

		/*
		* 加载或填充杂项数据
		*/
		void FillMiscData();

	private:
		inline static uint32_t smGenerateClumpMapThreadSizeInGroup = 8u;
		inline static uint32_t smBakeGrassBladeThreadSizeInGroup = 8u;
		inline static uint32_t smCullGrassBladeThreadSizeInGroup = 8u;
		inline static float smClumpMapSize{ 512.0f };
		inline static float smGrassClusterMeterSize{ 32.0f };			// 32 * 32 / m2 为一个GrassCluster
		inline static int32_t smMaxGrassBladeCountPerAxis{ 256 };		// 每个草群的每一个轴上的GrassBlade的个数
		inline static int32_t smVisibleGrassClusterCountPerAxis{ 8 };	// 可见的GrassCluster
		inline static float smVisibleDistance{ smGrassClusterMeterSize * smVisibleGrassClusterCountPerAxis / 2.0f  - smGrassClusterMeterSize / 2.0f };

		inline static uint32_t mMaxLODCount{ 2u };

		RenderEngine* mRenderEngine{ nullptr };
		TerrainSystem* mTerrainSystem{ nullptr };

		std::unique_ptr<VegetationDataCache> mDataCacher;
		std::unique_ptr<VegetationVirtualTable> mVegetationVirtualTable;

		std::vector<GpuGrassCluster> mNeedBakedGpuGrassClusters;	// 需要执行烘焙操作的GrassCluster
		std::vector<GpuGrassCluster> mVisibleGpuGrassClusters;		// 需要执行剔除操作的GrassCluster

		std::vector<ClumpParameter> mClumpParameters;

		GenerateClumpMapPassData mGenerateClumpMapPassData{};
		BakeGrassBladePassData mBakeGrassBladePassData{};
		CullGrassBladePassData mCullGrassBladePassData{};

		RenderGrassBladePassData mRenderLOD0GrassBladePassData{};
		RenderGrassBladePassData mRenderLOD1GrassBladePassData{};

		TextureWrap mGrassAlbedoMap;
		TextureWrap mGrassNormalMap;

		std::vector<GrassVertexAttribute> mLOD0GrassVertices;
		std::vector<uint32_t> mLOD0GrassIndices;
		BufferWrap mLOD0GrassVertexBuffer;
		BufferWrap mLOD0GrassIndexBuffer;

		std::vector<GrassVertexAttribute> mLOD1GrassVertices;
		std::vector<uint32_t> mLOD1GrassIndices;
		BufferWrap mLOD1GrassVertexBuffer;
		BufferWrap mLOD1GrassIndexBuffer;
	};

}