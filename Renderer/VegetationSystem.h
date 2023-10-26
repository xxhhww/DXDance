#pragma once
#include "Renderer/VegetationPhysicalDataCache.h"
#include "Renderer/VegetationVirtualTable.h"

namespace Renderer {

	/*
	* �ݵĶ�������(��������ɫ���ؽ��ݵĶ�������)
	*/
	struct GrassVertexAttribute {
	public:
		Math::Vector2 uv0;	// ����UV
		float t;			// ������ǰ��������Ҷ���Ӳݵĸ����ƶ��ľ���
		float side;			// ����ҶƬ�߳�(ʹ��ʱ��Ҫ�ָ��� [-1, 1]������)
	};

	struct GrassBlade {
	public:
		Math::Vector3 position;
		Math::Vector2 facing;

		float    hash;
		float    height;
		float    width;
		float    tilt;		// ������Ҷ����б״̬
		float    bend;		// ���Ʋ�Ҷ������(��ʵ���ǿ��Ʊ�������������)
		float    sideCurve;	// ���Ʋ�Ҷ�ıߵ�����
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
			uint32_t visibleGrassClusterListIndex;			// �ɼ���GrassCluster�б�����
			uint32_t bakedGrassBladeListIndex;				// �決��GrassBlade�б�
			uint32_t visibleLOD0GrassBladeIndexListIndex;	// �ɼ���GrassBlade����
			uint32_t visibleLOD1GrassBladeIndexListIndex;	// LOD1

			float grassResolution;
			float distanceCullStartDist{ 30.0f };
			float distanceCullEndDist{ 60.0f };
			float distanceCullMinimumGrassAmount{ 0.2f };
		};

		struct RenderGrassBladePassData {
		public:
			uint32_t bakedGrassBladeListIndex;				// �決��GrassBlade�б�
			uint32_t visibleGrassBladeIndexListIndex;		// �ɼ���GrassBlade����
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
		* �߼����£��������µ�GrassCluster���룬�ɵ�GrassCluster��������RenderEngine��Update��������
		*/
		void Update(const Math::Vector2& cameraPosition);

		void AddPass(RenderGraph& renderGraph);

	private:

		/*
		* ���ػ������������
		*/
		void FillMiscData();

	private:
		inline static uint32_t smGenerateClumpMapThreadSizeInGroup = 8u;
		inline static uint32_t smBakeGrassBladeThreadSizeInGroup = 8u;
		inline static uint32_t smCullGrassBladeThreadSizeInGroup = 8u;
		inline static float smClumpMapSize{ 512.0f };
		inline static float smGrassClusterMeterSize{ 32.0f };			// 32 * 32 / m2 Ϊһ��GrassCluster
		inline static int32_t smMaxGrassBladeCountPerAxis{ 256 };		// ÿ����Ⱥ��ÿһ�����ϵ�GrassBlade�ĸ���
		inline static int32_t smVisibleGrassClusterCountPerAxis{ 8 };	// �ɼ���GrassCluster
		inline static float smVisibleDistance{ smGrassClusterMeterSize * smVisibleGrassClusterCountPerAxis / 2.0f  - smGrassClusterMeterSize / 2.0f };

		inline static uint32_t mMaxLODCount{ 2u };

		RenderEngine* mRenderEngine{ nullptr };
		TerrainSystem* mTerrainSystem{ nullptr };

		std::unique_ptr<VegetationDataCache> mDataCacher;
		std::unique_ptr<VegetationVirtualTable> mVegetationVirtualTable;

		std::vector<GpuGrassCluster> mNeedBakedGpuGrassClusters;	// ��Ҫִ�к決������GrassCluster
		std::vector<GpuGrassCluster> mVisibleGpuGrassClusters;		// ��Ҫִ���޳�������GrassCluster

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