#ifndef _RvtHelper__
#define _RvtHelper__

struct DrawRvtTiledMapRequest {
	float4   tileRectInWorldSpace;
	float4   tileRectInImageSpace;
	float4x4 mvpMatrix;
};

struct DrawRvtLookUpMapRequest {
	int4     rect;		// Draw��Ŀ������(x,yΪx,y --- z,wΪwidth,height)
	int      mipLevel;
	float2   tilePos;	// ����ֵ

	float4x4 mvpMatrix;	// ת����ͼƬ�ռ��еľ���
};

#endif