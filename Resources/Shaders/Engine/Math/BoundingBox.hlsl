#ifndef _BoundingBox__
#define _BoundingBox__

struct BoundingBox {
    float4 minPosition;
    float4 maxPosition;
};

// 测试 点是否在平面的外侧
bool IsPositionOutSidePlane(float4 plane, float3 position) {
    return dot(plane.xyz, position) + plane.w < 0; 
}

// 测试AABB盒是否在平面外侧
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

// 使用摄像机的视锥体进行裁剪
bool FrustumCull(float4 plane[6], BoundingBox boundingBox) {
	return
	IsAABBOutSidePlane(plane[0], boundingBox.minPosition, boundingBox.maxPosition) ||
	IsAABBOutSidePlane(plane[1], boundingBox.minPosition, boundingBox.maxPosition) ||
	IsAABBOutSidePlane(plane[2], boundingBox.minPosition, boundingBox.maxPosition) ||
	IsAABBOutSidePlane(plane[3], boundingBox.minPosition, boundingBox.maxPosition) ||
	IsAABBOutSidePlane(plane[4], boundingBox.minPosition, boundingBox.maxPosition) ||
	IsAABBOutSidePlane(plane[5], boundingBox.minPosition, boundingBox.maxPosition);
}

// 二维平面下，判断点是否在矩形内部，以左下角为原点
bool IsPointInRect(float2 position, float4 region) {
    
	if ((position.x >= region.x) && (position.y >= region.y) && (position.x <= (region.x + region.z)) && (position.y <= (region.y + region.w))) {
		return true;
	}
	return false;
}

// 二维平面下，判断矩形rect1是否在矩形rect2内部，rect1为被判断的
bool IsRectInRect(float4 rect1, float4 rect2) {
	// 只需要判断矩形rect1的四个顶点是否在rect2内部
	return
	IsPointInRect(float2(rect1.x,           rect1.y          ), rect2) && 
	IsPointInRect(float2(rect1.x + rect1.z, rect1.y          ), rect2) &&
	IsPointInRect(float2(rect1.x,           rect1.y + rect1.w), rect2) &&
	IsPointInRect(float2(rect1.x + rect1.z, rect1.y + rect1.w), rect2);
}

#endif