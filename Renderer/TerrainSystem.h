#pragma once
#include "Renderer/ResourceAllocator.h"
#include "Renderer/RuntimeVirtualTextureSystem.h"
#include "Math/Int.h"

namespace Renderer {

	class RenderEngine;
	class RingFrameTracker;
	class RvtUpdater;
	class RuntimeVirtualTextureSystem;

	class TerrainSystem {
		friend class RvtUpdater;
		friend class RuntimeVirtualTextureSystem;

	public:
		struct NodeDescriptor {
			uint32_t isBranch = true;
			float pad1;
			float pad2;
			float pad3;
		};

		struct LODDescriptor {
			uint32_t nodeSize;			// ��LOD��ÿһ��Node�ı߳�(��)(Node��������)
			uint32_t nodeStartOffset;	// ��LOD�еĵ�һ��Node�Ŀ�ʼƫ����
			uint32_t nodeCount;			// ��LOD�е�Node���ܸ���
			float pad1;
		};

		struct TerrainBuilderPassData {
			Math::Vector4 nodeEvaluationC{ 1.2f, 0.0f, 0.0f, 0.0f };	// �û����ƵĽڵ�����ϵ��

			Math::Vector2 worldMeterSize{ 8192.0f, 8192.0f };				// ������XZ�᷽��Ĵ�С(��)
			uint32_t heightScale{ 4096u };
			uint32_t currPassLOD;

			uint32_t consumeNodeListIndex;
			uint32_t appendNodeListIndex;
			uint32_t finalNodeListIndex;
			uint32_t nodeDescriptorListIndex;

			uint32_t lodDescriptorListIndex;
			uint32_t culledPatchListIndex;
			uint32_t minmaxHeightMapIndex;
			uint32_t useFrustumCull{ 0u };

			uint32_t nearbyNodeListIndex;
			float pad1;
			float pad2;
			float pad3;
		};

		struct TerrainRendererPassData {
			// x: page table size
			// y: virtual texture size
			// z: max mipmap level
			// w: mipmap level bias
			Math::Vector4 vtFeedbackParams{};
			Math::Vector4 vtRealRect{};
			// x: padding size
			// y: tileSize
			// z: physical texture size x
			// w: physical texture size y
			Math::Vector4 vtPhysicalMapParams{};

			Math::Vector2 worldMeterSize{ 8192.0f, 8192.0f };
			uint32_t heightScale{ 4096u };
			uint32_t culledPatchListIndex;

			uint32_t heightMapIndex;
			uint32_t normalMapIndex;
			uint32_t lodDebug{ 0u };
			uint32_t splatMapIndex;

			uint32_t pageTableMapIndex;
			uint32_t physicalAlbedoMapIndex;
			uint32_t physicalNormalMapIndex;
			float pad1;

			uint32_t rChannelAlbedoMapIndex;
			uint32_t rChannelNormalMapIndex;
			uint32_t rChannelRoughnessMapIndex;
			uint32_t rChannelHeightMapIndex;

			uint32_t gChannelAlbedoMapIndex;
			uint32_t gChannelNormalMapIndex;
			uint32_t gChannelRoughnessMapIndex;
			uint32_t gChannelHeightMapIndex;

			uint32_t bChannelAlbedoMapIndex;
			uint32_t bChannelNormalMapIndex;
			uint32_t bChannelRoughnessMapIndex;
			uint32_t bChannelHeightMapIndex;

			uint32_t aChannelAlbedoMapIndex;
			uint32_t aChannelNormalMapIndex;
			uint32_t aChannelRoughnessMapIndex;
			uint32_t aChannelHeightMapIndex;
		};

		struct NodeLocation {
			uint32_t x;
			uint32_t y;
		};

		struct RenderPatch {
			Math::Vector2 position;
			Math::Vector2 minMaxHeight;
			uint32_t lod;
			float pad1;
			float pad2;
			float pad3;
		};

	public:
		bool isInitialized{ false };
		Math::Vector2 worldMeterSize{ 8192.0f, 8192.0f };
		float worldHeightScale{ 4096.0f };
		bool lodDebug{ 0u };
		bool useVT{ 1u };

		uint32_t maxLOD{ 4u };	// ���LOD�ȼ�
		uint32_t mostDetailNodeMeterSize{ 64u }; // �ϸ�Ľڵ�Ĵ�С(��λ: ��)

		std::vector<NodeDescriptor> nodeDescriptors;
		std::vector<LODDescriptor>  lodDescriptors;
		std::vector<NodeLocation>   maxLODNodeList;
		TerrainBuilderPassData  terrainBuilderPassData;
		TerrainRendererPassData terrainRendererPassData;

		std::unique_ptr<Renderer::Mesh> patchMesh;
		TextureWrap minmaxHeightMap;
		TextureWrap terrainHeightMap;
		TextureWrap terrainNormalMap;

		TextureWrap terrainSplatMap;	// ������

		// channel R
		TextureWrap grassAlbedoMap;
		TextureWrap grassNormalMap;
		TextureWrap grassRoughnessMap;
		TextureWrap grassHeightMap;

		// channel G
		TextureWrap mudAlbedoMap;
		TextureWrap mudNormalMap;
		TextureWrap mudRoughnessMap;
		TextureWrap mudHeightMap;

		// channel B
		TextureWrap cliffAlbedoMap;
		TextureWrap cliffNormalMap;
		TextureWrap cliffRoughnessMap;
		TextureWrap cliffHeightMap;

		// channel A
		TextureWrap snowAlbedoMap;
		TextureWrap snowNormalMap;
		TextureWrap snowRoughness;
		TextureWrap snowHeightMap;

	public:
		TerrainSystem(RenderEngine* renderEngine);
		~TerrainSystem();

		/*
		* ���γ�ʼ��
		*/
		void Initialize(RenderEngine* renderEngine);

		/*
		* ���TerrainPass��RenderGraph
		*/
		void AddPass(RenderEngine* renderEngine);

		/*
		* ��Ⱦ֡��ɺ�Ļص�����
		*/
		void FrameCompletedCallback(uint8_t frameIndex);

	private:
		void UpdateNodeAndLodDescriptorArray();

	private:
		RenderEngine* mRenderEngine{ nullptr };

		TextureWrap mTerrainFeedbackMap;
		struct QueuedReadbackFeedback {
		public:
			uint64_t renderFrameFenceValue{ 0u };	// �ñ�������Ⱦ���߳�д�룬ProcessFeedback�߳�ֻ��
			std::atomic<bool> isFresh{ false };		// �ñ�������Ⱦ���߳���ProcessFeedback�߳̽��з������޸�
		};
		/*
		* ��ÿһ����Ⱦ֡��ɺ���Ⱦ���̶߳������QueuedReadbackFeedback����ṹ������е�isFresh��������֪ͨProcessFeedback�߳̽��д���
		* ����ProcessFeedback�߳��У�Ҳ���isFresh���������޸�
		*/
		std::vector<QueuedReadbackFeedback> mQueuedReadbacks;
		std::vector<BufferWrap> mTerrainReadbackBuffers;

		std::unique_ptr<RuntimeVirtualTextureSystem> mRvtSystem;
		// RvtUpdater* mRvtUpdater{ nullptr };
	};

}