#pragma once
#include "Vector.h"
#include "Matrix.h"

namespace Math {

	struct Frustum {
	public:
		static void BuildFrustumPlanes(const Math::Matrix4& comboMatrix, Math::Vector4* outputPlanes);
	
		static void NormalizePlane(Math::Vector4& plane);
	};

}