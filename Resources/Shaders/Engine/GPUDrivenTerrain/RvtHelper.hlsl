#ifndef _RvtHelper__
#define _RvtHelper__

struct DrawRvtTiledMapRequest {
	float4   tileRectInWorldSpace;
	float4   tileRectInImageSpace;
	float4x4 mvpMatrix;

	float4 blendTile;
	float4 tileOffset;
};

struct DrawRvtLookUpMapRequest {
	float4   rect;		// Draw的目标区域(x,y为x,y --- z,w为width,height)
	int      mipLevel;
	float    pad;
	float2   tilePos;	// 索引值

	float4x4 mvpMatrix;	// 转换到图片空间中的矩阵
};

#endif