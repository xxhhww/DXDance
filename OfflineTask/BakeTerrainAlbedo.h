#pragma once
#include <DirectXTex/DirectXTex.h>
#include <string>
#include "Renderer/RenderEngine.h"
#include "Math/Vector.h"
#include "Math/Matrix.h"

namespace OfflineTask {

	struct BakeTerrainAlbedoPassData {
	public:
		uint32_t terrainHeightMapIndex;
		uint32_t terrainNormalMapIndex;
		uint32_t terrainSplatMapIndex;
		float    pad0;

		Math::Matrix4 mvpMatrix;

		Math::Vector4 tileOffset;
		Math::Vector4 blendOffset;
	};

	class BakeTerrainAlbedo {
	public:
		void Initialize(
			const std::string& heightMapFilepath,
			const std::string& normalMapFilepath,
			const std::string& splatMapFilepath,
			const std::string& albedoOutputPath,
			const std::string& normalOutputPath,
			Renderer::RenderEngine* renderEngine);

		void Generate(Renderer::CommandBuffer& commandBuffer, Renderer::RenderContext& renderContext);

		void OnCompleted();

	private:
		inline static float   smTerrainMeterSize = 8192.0f;				// 地形8192 * 8192
		inline static int32_t smTerrainMeshVertexCountPerAxis = 8193;	// 地形网格每个轴上的顶点个数8193
		inline static std::string smBakeTerrainTextureSN = "BakeTerrainTexture";

		Renderer::TextureWrap mTerrainHeightMap;
		Renderer::TextureWrap mTerrainNormalMap;
		Renderer::TextureWrap mTerrainSplatMap;

		Renderer::TextureWrap mOutputAlbedoMap;
		Renderer::BufferWrap  mOutputAlbedoReadbackBuffer;
		Renderer::TextureWrap mOutputNormalMap;
		Renderer::BufferWrap  mOutputNormalReadbackBuffer;

		uint32_t             mQuadMeshVertexCountPerAxis;
		Renderer::BufferWrap mQuadMeshVertexBuffer;
		Renderer::BufferWrap mQuadMeshIndexBuffer;
		uint32_t             mQuadMeshIndexCount;

		BakeTerrainAlbedoPassData mBakeTerrainAlbedoPassData;

	};

}