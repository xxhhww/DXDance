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
	* 草的顶点属性(用于在着色器重建草的顶点坐标)
	*/
	struct GrassVertexAttribute {
	public:
		Math::Vector2 uv0;	// 顶点UV
		float t;			// 描述当前顶点沿着叶脉从草的根部移动的距离
		float side;			// 描述叶片边长(使用时需要恢复到 [-1, 1]的区域)

	public:
		inline GrassVertexAttribute(const Math::Vector2& _uv0, float _t, float _side)
			: uv0(_uv0)
			, t(_t)
			, side(_side) {}

		inline ~GrassVertexAttribute() = default;
	};

	/*
	* 草点
	*/
	struct GrassBladeDescriptor {
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

	/*
	* 草的群落参数
	*/
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

	/*
	* 草地节点请求任务
	*/
	struct GrasslandNodeRequestTask {
	public:
		GrasslandLinearBufferCache::Node* cacheNode{ nullptr };	// 一个缓冲元素
		int32_t prevGrasslandNodeIndex{ -1 };		// 前任地形节点索引
		int32_t nextGrasslandNodeIndex{ -1 };		// 下任地形节点索引
	};

	/*
	* 草地节点描述
	*/
	struct GrasslandNodeDescriptor {
	public:
		uint32_t tileIndex = 65536;	// 65536表示资源未烘焙
		float pad1;
		float pad2;
		float pad3;
	};

	/*
	* 草地节点实时状态
	*/
	struct GrasslandNodeRuntimeState {
	public:
		bool inReady{ false };		// 是否在CPU端分配到一个TileCache
		bool inQueue{ false };		// 是否位于GPU命令提交队列中
		bool inLoading{ false };	// 是否GPU命令已被提交

		bool inReadyOut{ false };
		bool inQueueOut{ false };
		bool inLoadingOut{ false };

		bool inBuffer{ false };	// 是否已在LinearBuffer中中

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

		void AddPass();

		void ProduceGrasslandNodeRequest(std::vector<GrasslandNodeRequestTask>& requestTasks);

	private:
		RenderEngine* mRenderEngine{ nullptr };
		TerrainRenderer* mTerrainRenderer{ nullptr };

		TerrainSetting mTerrainSetting;

		std::vector<GrasslandNodeDescriptor>   mGrasslandNodeDescriptors;		// 草地全节点描述状态表(GPU端)
		std::vector<GrasslandNodeRuntimeState> mGrasslandNodeRuntimeStates;		// 草地全节点运行时状态(CPU端)

		std::unique_ptr<GrasslandLinearBuffer>      mGrasslandLinearBuffer;
		std::unique_ptr<GrasslandLinearBufferCache> mGrasslandLinearBufferCache;

		std::vector<GrasslandNodeRequestTask> mGrasslandNodeRequestTasks;

		struct GenerateClumpMapPassData {
		public:
			Math::Vector2 clumpMapSize{ 512.0f, 512.0f };
			uint32_t      numClumps{ 3u };
			uint32_t      clumpMapIndex;
		};
		std::vector<ClumpParameter> mClumpParameters;
		GenerateClumpMapPassData mGenerateClumpMapPassData;
		inline static uint32_t smGenerateClumpMapThreadSizeInGroup = 8u;

		struct BakeGrassBladePassData {
		public:
			Math::Vector2 terrainWorldMeterSize;
			float         heightScale;
			uint32_t      terrainHeightMapAtlasIndex;

			uint32_t      terrainNodeDescriptorListIndex;
			uint32_t      terrainLodDescriptorListIndex;
			uint32_t      visibleGrasslandNodeRequestTaskListIndex;
			uint32_t      grasslandMapIndex;

			uint32_t      grasslandLinearBufferIndex;
			uint32_t      grassResolution;
			float         jitterStrength{ 0.5f };
			float         pad1;

			uint32_t      clumpMapIndex;
			float         clumpMapScale{ 0.1f };
			uint32_t      clumpParameterBufferIndex;
			uint32_t      clumpParameterNums;
		};
		BakeGrassBladePassData mBakeGrassBladePassData;
		inline static uint32_t smBakeGrassBladeThreadSizeInGroup = 8u;


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