#include "Renderer/TerrainSetting.h"

namespace Renderer {

	bool     TerrainSetting::smUseFrustumCull = false;
	bool     TerrainSetting::smUseLodDebug = true;
	bool     TerrainSetting::smUseRenderCameraDebug = true;
	float	 TerrainSetting::smNodeEvaluationC = 1.2f;
	float	 TerrainSetting::smTerrainMeterSize = 8192.0f;
	float	 TerrainSetting::smTerrainHeightScale = 1325.0f;
	float	 TerrainSetting::smMinLODNodeMeterSize = 64.0f;		// LOD0��Ӧ�ĵؿ��СΪ64.0f
	float	 TerrainSetting::smMaxLODNodeMeterSize = 1024.0f;	// LOD4��Ӧ�ĵؿ��СΪ1024.0f
	uint32_t TerrainSetting::smMaxLOD = 4;						// pow(2, 4) * 64 = 1024.0f���� LOD0 1 2 3 4
	uint32_t TerrainSetting::smPatchCountPerNodePerAxis = 8;	// patch count
	int32_t  TerrainSetting::smTerrainDataLoadedRange = 4;		// �������ݼ��ط�Χ(�������)
	uint32_t TerrainSetting::smTerrainDataLoadedLimit = 16;		// һ�μ���16�����νڵ������

	uint32_t TerrainSetting::smFarTerrainTextureAtlasTileCountPerAxis = 25;	// ��������ͼ��ÿ�����Tile����
	uint32_t TerrainSetting::smTerrainTiledSplatMapTileCountPerCache = 512;
	int32_t  TerrainSetting::smTerrainTiledTextureDataLoadedRange = 8;			// 128 * 16 = 2048
	uint32_t TerrainSetting::smTerrainTiledTextureDataLoadedLimit = 16;			// һ�μ���16��Tile������

	uint32_t TerrainSetting::smTerrainTiledGrassLandMapTileCountPerCache = 512;
	int32_t  TerrainSetting::smTerrainTiledGrassLandMapDataLoadedRange = 4;		// 256 * 4 = 1024
	uint32_t TerrainSetting::smTerrainTiledGrassLandMapDataLoadedLimit = 8;		// һ�μ���8��Tile������

	uint32_t TerrainSetting::smTerrainFeedbackScale = 2;		// TerrainFeedback�����FinalOutput�����Ŵ�С
	uint32_t TerrainSetting::smRvtTileSizeNoPadding = 256;		// ʵʱ����������һ��Tile�Ĵ�С
	uint32_t TerrainSetting::smRvtTilePaddingSize = 4;			// ʵʱ����������һ��Tile������С
	uint32_t TerrainSetting::smRvtTileSizeWithPadding = smRvtTileSizeNoPadding + smRvtTilePaddingSize * 2u;
	uint32_t TerrainSetting::smRvtTileCountPerAxisInAtlas = 20;	// ʵʱ����������ʵ��������ÿ�����Tile����
	uint32_t TerrainSetting::smRvtAtlasTextureSize = smRvtTileSizeWithPadding * smRvtTileCountPerAxisInAtlas;
	float    TerrainSetting::smRvtRectRadius = 512;				// ʵʱ��������ľ��ΰ뾶(�߳���һ��)
	uint32_t TerrainSetting::smRvtPixelSizePerMeter = 64;		// ÿ�׵�����ֵ

	bool     TerrainSetting::smRvtUsePageLevelDebug = true;		// ��Ⱦʱ�Ƿ���ʾPageLevel
	uint32_t TerrainSetting::smRvtMaxPageLevel = 7;				// log2(smRvtRectRadius * 2)  7 => 512m 8=> 1024m
	uint32_t TerrainSetting::smRvtPageLevelBias = 0;
	uint32_t TerrainSetting::smRvtVirtualTextureSizeInBytesInPage0Level = smRvtRectRadius * 2 * smRvtPixelSizePerMeter;
	uint32_t TerrainSetting::smRvtTileCountPerAxisInPage0Level = (smRvtVirtualTextureSizeInBytesInPage0Level / smRvtTileSizeNoPadding);	// ��������ҳ��(���ұ�)�Ĵ�С
	uint32_t TerrainSetting::smRvtVirtualTextureSizeInBytesInWorld = smTerrainMeterSize * smRvtPixelSizePerMeter;
	uint32_t TerrainSetting::smRvtTileCountPerAxisInWorld = (smRvtVirtualTextureSizeInBytesInWorld / smRvtTileSizeNoPadding);			// ������ÿ�����Tile����

	float    TerrainSetting::smWorldMeterSizePerTileInPage0Level = (float)smRvtTileSizeNoPadding / (float)smRvtPixelSizePerMeter;
	float    TerrainSetting::smWorldMeterSizePerPaddingInPage0Level = (float)smRvtTilePaddingSize / (float)smRvtPixelSizePerMeter;

	uint32_t TerrainSetting::smTerrainFeedbackBufferElementCount = (smRvtTileCountPerAxisInPage0Level * smRvtTileCountPerAxisInPage0Level / 2);

	uint32_t TerrainSetting::smRvtDataLoadedLimit = 6;

	float    TerrainSetting::smRvtRealRectChangedViewDistance = 128;

	uint32_t TerrainSetting::smWorldMeterSizePerTiledTexture = 32;


	float    TerrainSetting::smGrasslandNodeMeterSize = 64.0f;
	uint32_t TerrainSetting::smGrasslandLinearBufferTileCount = 32;
	uint32_t TerrainSetting::smGrassBladePerAxis = 512;
	float    TerrainSetting::smClumpMapSize = 512.0f;
	int32_t  TerrainSetting::smGrasslandNodeBakedRange = 2;
}