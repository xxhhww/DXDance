#pragma once
#include "Math/Vector.h"
#include "Math/Matrix.h"

namespace Math {

	struct BoundingBox {
	public:
		Math::Vector4 minPosition{ (float)INT_MAX, (float)INT_MAX, (float)INT_MAX, 0.0f };
		Math::Vector4 maxPosition{ (float)INT_MIN, (float)INT_MIN, (float)INT_MIN, 0.0f };

		BoundingBox& operator+=(const Math::Vector3& position) {
			minPosition.x = (position.x < minPosition.x ? position.x : minPosition.x);
			minPosition.y = (position.y < minPosition.y ? position.y : minPosition.y);
			minPosition.z = (position.z < minPosition.z ? position.z : minPosition.z);

			maxPosition.x = (position.x > maxPosition.x ? position.x : maxPosition.x);
			maxPosition.y = (position.y > maxPosition.y ? position.y : maxPosition.y);
			maxPosition.z = (position.z > maxPosition.z ? position.z : maxPosition.z);

			return *this;
		}

		BoundingBox& operator+=(const Math::BoundingBox& boundingBox) {
			minPosition.x = (boundingBox.minPosition.x < minPosition.x ? boundingBox.minPosition.x : minPosition.x);
			minPosition.y = (boundingBox.minPosition.y < minPosition.y ? boundingBox.minPosition.y : minPosition.y);
			minPosition.z = (boundingBox.minPosition.z < minPosition.z ? boundingBox.minPosition.z : minPosition.z);

			maxPosition.x = (boundingBox.maxPosition.x > maxPosition.x ? boundingBox.maxPosition.x : maxPosition.x);
			maxPosition.y = (boundingBox.maxPosition.y > maxPosition.y ? boundingBox.maxPosition.y : maxPosition.y);
			maxPosition.z = (boundingBox.maxPosition.z > maxPosition.z ? boundingBox.maxPosition.z : maxPosition.z);

			return *this;
		}

		BoundingBox transformBy(const Math::Matrix4& transform) {
			BoundingBox newBoundingBox;

			const XMVECTOR VecMin = minPosition;
			const XMVECTOR VecMax = maxPosition;

			const XMVECTOR m0 = Math::Vector4(transform.m[0]);
			const XMVECTOR m1 = Math::Vector4(transform.m[1]);
			const XMVECTOR m2 = Math::Vector4(transform.m[2]);
			const XMVECTOR m3 = Math::Vector4(transform.m[3]);

			const XMVECTOR Half = Math::Vector3(0.5f, 0.5f, 0.5f); // VectorSetFloat1() can be faster than SetFloat3(0.5, 0.5, 0.5, 0.0). Okay if 4th element is 0.5, it's multiplied by 0.0 below and we discard W anyway.
			const XMVECTOR Origin = DirectX::XMVectorMultiply(DirectX::XMVectorAdd(VecMax, VecMin), Half);
			const XMVECTOR Extent = DirectX::XMVectorMultiply(DirectX::XMVectorSubtract(VecMax, VecMin), Half);

			XMVECTOR NewOrigin = DirectX::XMVectorMultiply(DirectX::XMVectorSplatX(Origin), m0);
			NewOrigin = DirectX::XMVectorMultiplyAdd(DirectX::XMVectorSplatY(Origin), m1, NewOrigin);
			NewOrigin = DirectX::XMVectorMultiplyAdd(DirectX::XMVectorSplatZ(Origin), m2, NewOrigin);
			NewOrigin = DirectX::XMVectorAdd(NewOrigin, m3);

			XMVECTOR NewExtent = DirectX::XMVectorAbs(DirectX::XMVectorMultiply(DirectX::XMVectorSplatX(Extent), m0));
			NewExtent = DirectX::XMVectorAdd(NewExtent, DirectX::XMVectorAbs(DirectX::XMVectorMultiply(DirectX::XMVectorSplatY(Extent), m1)));
			NewExtent = DirectX::XMVectorAdd(NewExtent, DirectX::XMVectorAbs(DirectX::XMVectorMultiply(DirectX::XMVectorSplatZ(Extent), m2)));

			const XMVECTOR NewVecMin = DirectX::XMVectorSubtract(NewOrigin, NewExtent);
			const XMVECTOR NewVecMax = DirectX::XMVectorAdd(NewOrigin, NewExtent);

			newBoundingBox.minPosition = NewVecMin;
			newBoundingBox.maxPosition = NewVecMax;

			return newBoundingBox;
		}

		Math::Vector3 GetExtent() const {
			return Math::Vector3{
				(maxPosition.x - minPosition.x) / 2.0f,
				(maxPosition.y - minPosition.y) / 2.0f,
				(maxPosition.z - minPosition.z) / 2.0f,
			};
		}

		Math::Vector3 GetCenter() const {
			return Math::Vector3{
				(minPosition.x + maxPosition.x) / 2.0f,
				(minPosition.y + maxPosition.y) / 2.0f,
				(minPosition.z + maxPosition.z) / 2.0f,
			};
		}
	};

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