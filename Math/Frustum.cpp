#include "Frustum.h"

namespace Math {

	void Frustum::BuildFrustumPlanes(const Math::Matrix4& comboMatrix, Math::Vector4* outputPlanes) {
		// Left clipping plane
		outputPlanes[0].x = comboMatrix._14 + comboMatrix._11;
		outputPlanes[0].y = comboMatrix._24 + comboMatrix._21;
		outputPlanes[0].z = comboMatrix._34 + comboMatrix._31;
		outputPlanes[0].w = comboMatrix._44 + comboMatrix._41;
		// Right clipping plane
		outputPlanes[1].x = comboMatrix._14 - comboMatrix._11;
		outputPlanes[1].y = comboMatrix._24 - comboMatrix._21;
		outputPlanes[1].z = comboMatrix._34 - comboMatrix._31;
		outputPlanes[1].w = comboMatrix._44 - comboMatrix._41;
		// Top clipping plane
		outputPlanes[2].x = comboMatrix._14 - comboMatrix._12;
		outputPlanes[2].y = comboMatrix._24 - comboMatrix._22;
		outputPlanes[2].z = comboMatrix._34 - comboMatrix._32;
		outputPlanes[2].w = comboMatrix._44 - comboMatrix._42;
		// Bottom clipping plane
		outputPlanes[3].x = comboMatrix._14 + comboMatrix._12;
		outputPlanes[3].y = comboMatrix._24 + comboMatrix._22;
		outputPlanes[3].z = comboMatrix._34 + comboMatrix._32;
		outputPlanes[3].w = comboMatrix._44 + comboMatrix._42;
		// Near clipping plane
		outputPlanes[4].x = comboMatrix._13;
		outputPlanes[4].y = comboMatrix._23;
		outputPlanes[4].z = comboMatrix._33;
		outputPlanes[4].w = comboMatrix._43;
		// Far clipping plane
		outputPlanes[5].x = comboMatrix._14 - comboMatrix._13;
		outputPlanes[5].y = comboMatrix._24 - comboMatrix._23;
		outputPlanes[5].z = comboMatrix._34 - comboMatrix._33;
		outputPlanes[5].w = comboMatrix._44 - comboMatrix._43;

		NormalizePlane(outputPlanes[0]);
		NormalizePlane(outputPlanes[1]);
		NormalizePlane(outputPlanes[2]);
		NormalizePlane(outputPlanes[3]);
		NormalizePlane(outputPlanes[4]);
		NormalizePlane(outputPlanes[5]);
	}

	void Frustum::NormalizePlane(Math::Vector4& plane) {
		float mag = sqrt(plane.x * plane.x + plane.y * plane.y + plane.z * plane.z);
		plane.x = plane.x / mag;
		plane.y = plane.y / mag;
		plane.z = plane.z / mag;
		plane.w = plane.w / mag;
	}

}