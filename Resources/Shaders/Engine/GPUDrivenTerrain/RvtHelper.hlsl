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
	float4   rect;		// Draw��Ŀ������(x,yΪx,y --- z,wΪwidth,height)
	int      mipLevel;
	float    pad;
	float2   tilePos;	// ����ֵ

	float4x4 mvpMatrix;	// ת����ͼƬ�ռ��еľ���
};

#endif