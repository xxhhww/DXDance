#ifndef _BoundingBox__
#define _BoundingBox__

struct BoundingBox {
    float4 minPosition;
    float4 maxPosition;
};

// ���� ���Ƿ���ƽ������
bool IsPositionOutSidePlane(float4 plane, float3 position) {
    return dot(plane.xyz, position) + plane.w < 0; 
}

// ����AABB���Ƿ���ƽ�����
bool IsAABBOutSidePlane(float4 plane, float4 minPosition, float4 maxPosition) {
    return 
	IsPositionOutSidePlane(plane, minPosition.xyz) &&
    IsPositionOutSidePlane(plane, maxPosition.xyz) &&
    IsPositionOutSidePlane(plane, float3(minPosition.x, minPosition.y, maxPosition.z)) &&
    IsPositionOutSidePlane(plane, float3(minPosition.x, maxPosition.y, minPosition.z)) &&
    IsPositionOutSidePlane(plane, float3(minPosition.x, maxPosition.y, maxPosition.z)) &&
    IsPositionOutSidePlane(plane, float3(maxPosition.x, minPosition.y, maxPosition.z)) &&
    IsPositionOutSidePlane(plane, float3(maxPosition.x, maxPosition.y, minPosition.z)) &&
    IsPositionOutSidePlane(plane, float3(maxPosition.x, minPosition.y, minPosition.z));
}

// ʹ�����������׶����вü�
bool FrustumCull(float4 plane[6], BoundingBox boundingBox) {
	return
	IsAABBOutSidePlane(plane[0], boundingBox.minPosition, boundingBox.maxPosition) ||
	IsAABBOutSidePlane(plane[1], boundingBox.minPosition, boundingBox.maxPosition) ||
	IsAABBOutSidePlane(plane[2], boundingBox.minPosition, boundingBox.maxPosition) ||
	IsAABBOutSidePlane(plane[3], boundingBox.minPosition, boundingBox.maxPosition) ||
	IsAABBOutSidePlane(plane[4], boundingBox.minPosition, boundingBox.maxPosition) ||
	IsAABBOutSidePlane(plane[5], boundingBox.minPosition, boundingBox.maxPosition);
}

// ��άƽ���£��жϵ��Ƿ��ھ����ڲ��������½�Ϊԭ��
bool IsPointInRect(float2 position, float4 region) {
    
	if ((position.x >= region.x) && (position.y >= region.y) && (position.x <= (region.x + region.z)) && (position.y <= (region.y + region.w))) {
		return true;
	}
	return false;
}

// ��άƽ���£��жϾ���rect1�Ƿ��ھ���rect2�ڲ���rect1Ϊ���жϵ�
bool IsRectInRect(float4 rect1, float4 rect2) {
	// ֻ��Ҫ�жϾ���rect1���ĸ������Ƿ���rect2�ڲ�
	return
	IsPointInRect(float2(rect1.x,           rect1.y          ), rect2) && 
	IsPointInRect(float2(rect1.x + rect1.z, rect1.y          ), rect2) &&
	IsPointInRect(float2(rect1.x,           rect1.y + rect1.w), rect2) &&
	IsPointInRect(float2(rect1.x + rect1.z, rect1.y + rect1.w), rect2);
}

#endif