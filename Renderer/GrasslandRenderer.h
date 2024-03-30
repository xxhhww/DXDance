#pragma once
#include <vector>
#include "Renderer/TerrainSetting.h"
#include "Renderer/GrasslandLinearBuffer.h"
#include "Renderer/GrasslandLinearBufferCache.h"

namespace Renderer {

	class RenderEngine;
	class TerrainRenderer;
	class GrasslandLinearBuffer;

	/*
	* �ݵĶ�������(��������ɫ���ؽ��ݵĶ�������)
	*/
	struct GrassVertex {
	public:
		Math::Vector2 uv0;	// ����UV
		float t;			// ������ǰ��������Ҷ���Ӳݵĸ����ƶ��ľ���
		float side;			// ����ҶƬ�߳�(ʹ��ʱ��Ҫ�ָ��� [-1, 1]������)

	public:
		inline GrassVertex(const Math::Vector2& _uv0, float _t, float _side)
			: uv0(_uv0)
			, t(_t)
			, side(_side) {}

		inline ~GrassVertex() = default;
	};

	/*
	* �ݵ�
	*/
	struct GrassBladeDescriptor {
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

	/*
	* �ݵ�Ⱥ�����
	*/
	struct GrassClumpParameter {
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

	/*
	* �ݵؽڵ���������
	*/
	struct GrasslandNodeRequestTask {
	public:
		GrasslandLinearBufferCache::Node* cacheNode{ nullptr };	// һ������Ԫ��
		int32_t prevGrasslandNodeIndex{ -1 };		// ǰ�ε��νڵ�����
		int32_t nextGrasslandNodeIndex{ -1 };		// ���ε��νڵ�����
	};

	/*
	* �ݵؽڵ�����
	*/
	struct GrasslandNodeDescriptor {
	public:
		uint32_t tileIndex = 255;	// 255��ʾ��Դδ�決
		uint32_t nodeLocationX;
		uint32_t nodeLocationY;
		float    nodeMeterSize;
	};

	/*
	* �ݵؽڵ�ʵʱ״̬
	*/
	struct GrasslandNodeRuntimeState {
	public:
		bool inReady{ false };		// �Ƿ���CPU�˷��䵽һ��TileCache
		bool inQueue{ false };		// �Ƿ�λ��GPU�����ύ������
		bool inLoading{ false };	// �Ƿ�GPU�����ѱ��ύ

		bool inReadyOut{ false };
		bool inQueueOut{ false };
		bool inLoadingOut{ false };

		bool inBuffer{ false };	// �Ƿ�����LinearBuffer����

		GrasslandLinearBufferCache::Node* cacheNode{ nullptr };

	public:
		inline void SetInReady() { inReady = true;  inQueue = false; inLoading = false; inReadyOut = false; inQueueOut = false; inLoadingOut = false; inBuffer = false; }
		inline void SetInQueue() { inReady = false; inQueue = true;  inLoading = false; inReadyOut = false; inQueueOut = false; inLoadingOut = false; inBuffer = false; }
		inline void SetInLoading() { inReady = false; inQueue = false; inLoading = true;  inReadyOut = false; inQueueOut = false; inLoadingOut = false; inBuffer = false; }

		inline void SetInReadyOut() { inReady = false; inQueue = false; inLoading = false; inReadyOut = true;  inQueueOut = false; inLoadingOut = false; inBuffer = true; }
		inline void SetInQueueOut() { inReady = false; inQueue = false; inLoading = false; inReadyOut = false; inQueueOut = true;  inLoadingOut = false; inBuffer = true; }
		inline void SetInLoadingOut() { inReady = false; inQueue = false; inLoading = false; inReadyOut = false; inQueueOut = false; inLoadingOut = true;  inBuffer = true; }

		inline void SetInTexture() { inReady = false; inQueue = false; inLoading = false; inReadyOut = false;  inQueueOut = false; inLoadingOut = false; inBuffer = true; }
		inline void SetOutTexture() { inReady = false; inQueue = false; inLoading = false; inReadyOut = false;  inQueueOut = false; inLoadingOut = false; inBuffer = false; }
	};

	class GrasslandRenderer {
		friend class GrasslandLinearBuffer;
	public:
		GrasslandRenderer(RenderEngine* renderEngine, TerrainRenderer* terrainRenderer);
		~GrasslandRenderer();

		void Initialize();

		void Update();

		void AddPass();

		// requestTasks: ��Ҫִ�к決�������б�, visibleGrasslandNode: �ɼ���GrasslandNodes�������б�
		void ProduceGrasslandNodeRequest(std::vector<GrasslandNodeRequestTask>& requestTasks, std::vector<int32_t>& visibleGrasslandNodes);

	private:
		RenderEngine* mRenderEngine{ nullptr };
		TerrainRenderer* mTerrainRenderer{ nullptr };
		inline static bool smInitialized{ false };

		TerrainSetting mTerrainSetting;

		std::vector<GrasslandNodeDescriptor>   mGrasslandNodeDescriptors;		// �ݵ�ȫ�ڵ�����״̬��(GPU��)
		BufferWrap mGrasslandNodeDescriptorsBuffer;

		std::vector<GrasslandNodeRuntimeState> mGrasslandNodeRuntimeStates;		// �ݵ�ȫ�ڵ�����ʱ״̬(CPU��)

		std::unique_ptr<GrasslandLinearBuffer>      mGrasslandLinearBuffer;
		std::unique_ptr<GrasslandLinearBufferCache> mGrasslandLinearBufferCache;

		std::vector<GrasslandNodeRequestTask> mGrasslandNodeRequestTasks;

		struct GenerateClumpMapPassData {
		public:
			Math::Vector2 clumpMapSize{ 512.0f, 512.0f };
			uint32_t      numClumps{ 3u };
			uint32_t      clumpMapIndex;
		};
		std::vector<GrassClumpParameter> mClumpParameters;
		GenerateClumpMapPassData mGenerateClumpMapPassData;
		inline static uint32_t smGenerateClumpMapThreadSizeInGroup = 8u;

		struct BakeGrassBladePassData {
		public:
			Math::Vector2 terrainWorldMeterSize;
			float         heightScale;
			uint32_t      terrainHeightMapAtlasIndex;

			uint32_t      terrainNodeDescriptorListIndex;
			uint32_t      terrainLodDescriptorListIndex;
			uint32_t      grasslandNodeRequestTaskListIndex;
			uint32_t      grasslandMapIndex;

			uint32_t      grasslandNodeDescriptorListIndex;
			uint32_t      grasslandLinearBufferIndex;
			uint32_t      grassResolution;
			float         jitterStrength{ 0.5f };

			uint32_t      clumpMapIndex;
			float         clumpMapScale{ 0.1f };
			uint32_t      clumpParameterBufferIndex;
			uint32_t      clumpParameterNums;

			float         grasslandNodeMeterSize;
			uint32_t      terrainAtlasTileCountPerAxis;
			uint32_t      terrainAtlasTileWidthInPixels;
			float         meterSpacingBetweenTerrainAtlasTilePixelsInLod0;
		};
		BakeGrassBladePassData mBakeGrassBladePassData;
		inline static uint32_t smBakeGrassBladeThreadSizeInGroup = 8u;

		struct CullGrassBladePassData {
		public:
			uint32_t visibleGrasslandNodeListIndex;
			uint32_t grasslandLinearBufferIndex;
			uint32_t visibleLOD0GrassBladeIndexListIndex;
			uint32_t visibleLOD1GrassBladeIndexListIndex;

			float grassResolution;
			float distanceCullStartDist{ 30.0f };
			float distanceCullEndDist{ 60.0f };
			float distanceCullMinimumGrassAmount{ 0.2f };
		};
		CullGrassBladePassData mCullGrassBladePassData;
		inline static uint32_t smCullGrassBladeThreadSizeInGroup = 8u;

		std::vector<GrassVertex> mLOD0GrassVertices;
		std::vector<uint32_t> mLOD0GrassIndices;
		BufferWrap mLOD0GrassVertexBuffer;
		BufferWrap mLOD0GrassIndexBuffer;

		std::vector<GrassVertex> mLOD1GrassVertices;
		std::vector<uint32_t> mLOD1GrassIndices;
		BufferWrap mLOD1GrassVertexBuffer;
		BufferWrap mLOD1GrassIndexBuffer;

		/*
		* GPU����������
		*/
		struct GpuGrasslandNodeRequestTask {
		public:
			uint32_t prevGrasslandNodeIndex{ 65536u };	// ǰ�ε��νڵ�����
			uint32_t nextGrasslandNodeIndex;			// ���ε��νڵ�����

			uint32_t tileIndex;
		};
		std::vector<GrasslandNodeRequestTask> mCurrFrameGrasslandNodeRequestTasks;
		std::vector<GpuGrasslandNodeRequestTask> mCurrFrameGpuGrasslandNodeRequestTasks;
		std::vector<int32_t> mCurrFrameVisibleGrasslandNodes;
	};

}