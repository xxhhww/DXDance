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


		static uint32_t smTerrainFeedbackScale;				// TerrainFeedback�����FinalOutput�����Ŵ�С
		static uint32_t smRvtTileSizeNoPadding;				// ʵʱ����������һ��Tile�Ĵ�С
		static uint32_t smRvtTilePaddingSize;				// ʵʱ����������һ��Tile������С
		static uint32_t smRvtTileCountPerAxisInAtlas;		// ʵʱ��������������ͼ����ÿ�����Tile����
		static int32_t  smRvtRectRadius;					// ʵʱ��������ľ��ΰ뾶(�߳���һ��)
		static uint32_t smRvtPixelSizePerMeter;				// ÿ�׵�����ֵ

		static uint32_t smRvtMaxPageLevel;
		static uint32_t smRvtPageLevelBias;
		static uint32_t smRvtVirtualTextureSizeInBytesInPage0Level;	// VT��СInPage0Level
		static uint32_t smRvtTileCountPerAxisInPage0Level;			// ��������ҳ��(���ұ�)�Ĵ�С
		static uint32_t smRvtVirtualTextureSizeInBytesInWorld;		// VT��СInWorld
		static uint32_t smRvtTileCountPerAxisInWorld;				// ������ÿ�����Tile����
	};

}