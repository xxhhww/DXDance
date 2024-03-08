#pragma once
#include <cstdint>

namespace Renderer {

	/*
	* ��������
	*/
	struct TerrainSetting {
	public:
		static bool     smUseFrustumCull;
		static bool     smUseLodDebug;
		static bool     smUseRenderCameraDebug;
		static float	smNodeEvaluationC;
		static float	smTerrainMeterSize;
		static float	smTerrainHeightScale;
		static float	smMinLODNodeMeterSize;			// LOD0��Ӧ�ĵؿ��СΪ64.0f
		static float	smMaxLODNodeMeterSize;			// LOD4��Ӧ�ĵؿ��СΪ1024.0f
		static uint32_t	smMaxLOD;						// pow(2, 4) * 64 = 1024.0f���� LOD0 1 2 3 4
		static uint32_t smPatchCountPerNodePerAxis;		// patch count
		static int32_t  smTerrainDataLoadedRange;		// �������ݼ��صķ�Χ
		static uint32_t smTerrainDataLoadedLimit;		// һ�μ��ض��ٸ��ڵ������

		static uint32_t smFarTerrainTextureAtlasTileCountPerAxis;	// ��������ͼ��ÿ�����Tile����
		static uint32_t smTerrainTiledSplatMapTileCountPerCache;	// TiledSplatMap��ౣ������Tile
		static int32_t  smTerrainTiledTextureDataLoadedRange;		// TiledTextureTile�ļ��ط�Χ
		static uint32_t smTerrainTiledTextureDataLoadedLimit;		// TiledTextureһ�μ��ض��ٸ�Tile������

		static uint32_t smTerrainTiledGrassLandMapTileCountPerCache;
		static int32_t  smTerrainTiledGrassLandMapDataLoadedRange;
		static uint32_t smTerrainTiledGrassLandMapDataLoadedLimit;

		static uint32_t smTerrainFeedbackScale;				// TerrainFeedback�����FinalOutput�����Ŵ�С
		static uint32_t smRvtTileSizeNoPadding;				// ʵʱ����������һ��Tile�Ĵ�С
		static uint32_t smRvtTilePaddingSize;				// ʵʱ����������һ��Tile������С
		static uint32_t smRvtTileSizeWithPadding;
		static uint32_t smRvtTileCountPerAxisInAtlas;		// ʵʱ��������������ͼ����ÿ�����Tile����
		static uint32_t smRvtAtlasTextureSize;				// ����ͼ���Ĵ�С
		static float    smRvtRectRadius;					// ʵʱ��������ľ��ΰ뾶(�߳���һ��)
		static uint32_t smRvtPixelSizePerMeter;				// ÿ�׵�����ֵ

		static bool     smRvtUsePageLevelDebug;						// ��Ⱦʱ�Ƿ���ʾPageLevel
		static uint32_t smRvtMaxPageLevel;
		static uint32_t smRvtPageLevelBias;
		static uint32_t smRvtVirtualTextureSizeInBytesInPage0Level;	// VT��СInPage0Level
		static uint32_t smRvtTileCountPerAxisInPage0Level;			// ��������ҳ��(���ұ�)�Ĵ�С
		static uint32_t smRvtVirtualTextureSizeInBytesInWorld;		// VT��СInWorld
		static uint32_t smRvtTileCountPerAxisInWorld;				// ������ÿ�����Tile����

		static float	 smWorldMeterSizePerTileInPage0Level;		// ����ͼ��Ԫ�ض�Ӧ�������С
		static float    smWorldMeterSizePerPaddingInPage0Level;		// ͼ��Ԫ�ص�Padding��Ӧ�������С

		static uint32_t smTerrainFeedbackBufferElementCount;		// TerrainFeedbackBuffer��Ԫ�صĸ���

		static uint32_t smRvtDataLoadedLimit;						// һ�μ��ض��ٸ���������

		static float smRvtRealRectChangedViewDistance;				// Rvt����һ�ζ�Ӧ������ƶ��Ĵ�С

		static uint32_t smWorldMeterSizePerTiledTexture;			// �Ե��ν�������ƽ�̣�������ƽ��һ�����������˴��ĵ��������ǵ������������е�����
	};

}