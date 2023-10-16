#pragma once
#include "Renderer/RenderEngine.h"
#include <string>

namespace OfflineTask {

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

	struct GenerateClumpMapPassData {
	public:
		Math::Vector2 clumpMapSize{ 512.0f, 512.0f };
		uint32_t      numClumps{ 3u };
		uint32_t      clumpMapIndex;
	};

	struct GenerateGrassBladePassData {
	public:
		Math::Vector4 terrainTileRect;		// ���ο���Σ�ǰ���������ǵ��ο����½ǵ�ԭ�㣬�����������ǵؿ�ĳ���(��32 * 32Ϊһ����λ)

		Math::Vector2 terrainWorldMeterSize{ 8192.0f, 8192.0f };
		uint32_t      terrainHeightMapIndex;
		float         heightScale{ 1325.0f };

		uint32_t      clumpMapIndex;
		float         clumpMapScale{ 0.1f };
		uint32_t      clumpParameterBufferIndex;
		uint32_t      clumpParameterNums{ 3u };

		uint32_t      grassBladeBufferIndex;
		uint32_t      grassResolution{ 240u };
		float         centerColorSmoothStepLower{ 0.0f };
		float         centerColorSmoothStepUpper{ 6.69f };

		float         jitterStrength{ 5.0f };
		uint32_t      grassLayerMapIndex;
		float         pad1;
		float         pad2;
	};

	struct BakedGrassBlade {
	public:
		Math::Vector3 position;
		Math::Vector2 facing;

		float    hash;
		uint32_t type;

		float    height;
		float    width;
		float    tilt;		// ������Ҷ����б״̬
		float    bend;		// ���Ʋ�Ҷ������(��ʵ���ǿ��Ʊ�������������)
		float    sideCurve;	// ���Ʋ�Ҷ�ıߵ�����
	};

	class GenerateGrassBlade {
	public:
		void Initialize(const std::string& heightMapFilepath, const std::string& grassLayerFilepath, Renderer::RenderEngine* renderEngine);

		void Generate(Renderer::CommandBuffer& commandBuffer, Renderer::RenderContext& renderContext);

		void OnCompleted();

	private:
		void FillClumpParameters();
	
		Renderer::TextureDesc GetTextureDesc(const DirectX::TexMetadata& metadata);

		DirectX::TexMetadata GetTexMetadata(const Renderer::TextureDesc& textureDesc);

	private:
		inline static uint32_t smThreadSizeInGroup = 8u; // 8 * 8 * 1 as a thread group
		inline static float smClumpMapSize{ 512.0f };
		inline static float smWorldMeterSize{ 8192.0f };
		inline static float smGrassClusterMeterSize{ 32.0f };	// һ����Ⱥ�Ĵ�С
		inline static float smGrassClusterCountPerAxis{ smWorldMeterSize / smGrassClusterMeterSize };	// ÿ����ļ�Ⱥ�ĸ���
		inline static uint32_t smGrassResolution{ 240u };	// ��Ⱥ�вݵķֲ�����
		inline static bool smFirstFrame{ true };

		GHL::Device* mDevice{ nullptr };

		std::unique_ptr<Renderer::Texture> mTerrainHeightMap;
		std::unique_ptr<Renderer::Texture> mTerrainGrassLayerMap;

		std::vector<ClumpParameter> mClumpParameters;
		std::unique_ptr<Renderer::Buffer> mClumpParametersBuffer;

		std::unique_ptr<Renderer::Texture> mClumpMap;
		std::unique_ptr<Renderer::Buffer> mClumpMapReadback;
		
		// ÿһ֡�����޸�������
		std::unique_ptr<Renderer::Buffer> mGrassBladesBuffer;
		std::unique_ptr<Renderer::Buffer> mGrassBladesBufferReadback;
		std::unique_ptr<Renderer::Buffer> mGrassBladesCountBufferReadback;

		GenerateClumpMapPassData mGenerateClumpMapPassData{};
		GenerateGrassBladePassData mGenerateGrassBladePassData{};

		uint32_t currRowIndex = 0u;	// ��ǰ�����������
		uint32_t currColIndex = 0u;	// ��ǰ�����������
	};

}