#pragma once
#include <DirectXTex/DirectXTex.h>
#include <string>

#include "Renderer/RenderEngine.h"
#include "Math/Vector.h"
#include "Math/Matrix.h"

namespace OfflineTask {

	struct BakeTerrainTexturePassData {
	public:
		uint32_t terrainHeightMapIndex;
		uint32_t terrainNormalMapIndex;
		uint32_t terrainSplatMapIndex;
		float    worldMeterSizePerTiledTexture;

		uint32_t terrainAlbedoTextureArrayIndex;
		uint32_t terrainNormalTextureArrayIndex;
		uint32_t outputAlbedoMapIndex;
		uint32_t outputNormalMapIndex;

		Math::Matrix4 mvpMatrix;

		Math::Vector4 tileOffset;
		Math::Vector4 blendOffset;

		uint32_t  vertexCountPerAxis;
		float     vertexSpaceInMeterSize;   // 地形两个顶点之间的间隔
		float     terrainMeterSize;
		float     terrainHeightScale;
	};

	class TerrainTextureBaker {
	public:
		void Initialize(
			const std::string& heightMapFilepath,
			const std::string& splatMapFilepath,
			const std::string& terrainAlbedoTextureArrayFilepath,
			const std::string& terrainNormalTextureArrayFilepath,
			const std::string& terrainNormalMapPath,
			const std::string& albedoOutputPath,
			const std::string& normalOutputPath,
			Renderer::RenderEngine* renderEngine);

		void Generate(Renderer::CommandBuffer& commandBuffer, Renderer::RenderContext& renderContext);

		void OnCompleted();

		DirectX::TexMetadata GetTexMetadata(const Renderer::TextureDesc& textureDesc);

	private:
		inline static float   smTerrainMeterSize = 8192.0f;				// 地形8192 * 8192
		inline static int32_t smVertexCountPerAxis = 8193;				// 地形网格每个轴上的顶点个数8193
		inline static float   smVertexSpaceInMeterSize = 1.0f;			// 地形两个顶点之间的间隔
		inline static float   smTerrainHeightScale = 1325.0f;
		inline static float   smMeterSizePerTiledTexture = 32.0f;		// 地形纹理平铺，一个纹理平铺

		inline static uint32_t smThreadSizeInGroup = 8u;

		inline static std::string smBakeTerrainNormalMapSN = "BakeTerrainNormalMap";
		inline static std::string smBakeFarTerrainTextureSN = "BakeFarTerrainTexture";

		GHL::Device* mDevice{ nullptr };

		Renderer::TextureWrap mTerrainHeightMap;
		Renderer::TextureWrap mTerrainSplatMap;
		Renderer::TextureWrap mTerrainAlbedoTextureArray;
		Renderer::TextureWrap mTerrainNormalTextureArray;

		std::string mTerrainNormalMapPath;
		Renderer::TextureWrap mTerrainNormalMap;
		Renderer::BufferWrap  mTerrainNormalMapReadbackBuffer;

		std::string mOutputAlbedoMapPath;
		Renderer::TextureWrap mOutputAlbedoMap;
		Renderer::BufferWrap  mOutputAlbedoMapReadbackBuffer;

		Renderer::TextureWrap mOutputNormalMap;
		Renderer::BufferWrap  mOutputNormalMapReadbackBuffer;

		uint32_t             mQuadMeshVertexCountPerAxis;
		Renderer::BufferWrap mQuadMeshVertexBuffer;
		Renderer::BufferWrap mQuadMeshIndexBuffer;
		uint32_t             mQuadMeshIndexCount;

		BakeTerrainTexturePassData mBakeTerrainTexturePassData;
	};

}