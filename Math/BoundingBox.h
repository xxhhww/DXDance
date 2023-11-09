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
		}

		BoundingBox& operator+=(const Math::BoundingBox& boundingBox) {
			minPosition.x = (boundingBox.minPosition.x < minPosition.x ? boundingBox.minPosition.x : minPosition.x);
			minPosition.y = (boundingBox.minPosition.y < minPosition.y ? boundingBox.minPosition.y : minPosition.y);
			minPosition.z = (boundingBox.minPosition.z < minPosition.z ? boundingBox.minPosition.z : minPosition.z);

			maxPosition.x = (boundingBox.maxPosition.x > maxPosition.x ? boundingBox.maxPosition.x : maxPosition.x);
			maxPosition.y = (boundingBox.maxPosition.y > maxPosition.y ? boundingBox.maxPosition.y : maxPosition.y);
			maxPosition.z = (boundingBox.maxPosition.z > maxPosition.z ? boundingBox.maxPosition.z : maxPosition.z);
		}

		BoundingBox transformBy(const Math::Matrix4& transform) {

		}
	};

}