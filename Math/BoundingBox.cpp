#include "Math/BoundingBox.h"

namespace Math {

	static bool IsPositionOutSidePlane(Math::Vector4 plane, Math::Vector3 position) {
		return Math::Vector3{ plane.x, plane.y, plane.z }.Dot(position) + plane.w < 0;
	}

	// 测试AABB盒是否在平面外侧
	static bool IsAABBOutSidePlane(Math::Vector4 plane, Math::Vector4 minPosition, Math::Vector4 maxPosition) {
		return
			IsPositionOutSidePlane(plane, Math::Vector3{ minPosition.x, minPosition.y, minPosition.z }) &&
			IsPositionOutSidePlane(plane, Math::Vector3{ maxPosition.x, maxPosition.y, maxPosition.z }) &&
			IsPositionOutSidePlane(plane, Math::Vector3{ minPosition.x, minPosition.y, maxPosition.z }) &&
			IsPositionOutSidePlane(plane, Math::Vector3{ minPosition.x, maxPosition.y, minPosition.z }) &&
			IsPositionOutSidePlane(plane, Math::Vector3{ minPosition.x, maxPosition.y, maxPosition.z }) &&
			IsPositionOutSidePlane(plane, Math::Vector3{ maxPosition.x, minPosition.y, maxPosition.z }) &&
			IsPositionOutSidePlane(plane, Math::Vector3{ maxPosition.x, maxPosition.y, minPosition.z }) &&
			IsPositionOutSidePlane(plane, Math::Vector3{ maxPosition.x, minPosition.y, minPosition.z });
	}

	// 使用摄像机的视锥体进行裁剪
	static bool FrustumCull(Math::Vector4 plane[6], BoundingBox boundingBox) {
		return
			IsAABBOutSidePlane(plane[0], boundingBox.minPosition, boundingBox.maxPosition) ||
			IsAABBOutSidePlane(plane[1], boundingBox.minPosition, boundingBox.maxPosition) ||
			IsAABBOutSidePlane(plane[2], boundingBox.minPosition, boundingBox.maxPosition) ||
			IsAABBOutSidePlane(plane[3], boundingBox.minPosition, boundingBox.maxPosition) ||
			IsAABBOutSidePlane(plane[4], boundingBox.minPosition, boundingBox.maxPosition) ||
			IsAABBOutSidePlane(plane[5], boundingBox.minPosition, boundingBox.maxPosition);
	}

}