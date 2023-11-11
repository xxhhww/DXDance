#pragma once
#include "Math/Vector.h"
#include "Math/Matrix.h"

namespace Math {

	struct BoundingBox {
	public:
		Math::Vector3 minPosition{ INT_MAX, INT_MAX, INT_MAX };
		Math::Vector3 maxPosition{ INT_MIN, INT_MIN, INT_MIN };

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
	};

}