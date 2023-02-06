#include "Matrix.h"
#include "Vector.h"
#include "Quaternion.h"

namespace Math {

	Matrix4::Matrix4(const Vector3& translation, const Quaternion& rotation, const Vector3& scaling) {
		XMStoreFloat4x4(this, XMMatrixAffineTransformation(
			scaling, XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), rotation, translation
		));
	}

	Vector4 Matrix4::x() const { return Vector4(this->m[0]); }
	Vector4 Matrix4::y() const { return Vector4(this->m[1]); }
	Vector4 Matrix4::z() const { return Vector4(this->m[2]); }
	Vector4 Matrix4::w() const { return Vector4(this->m[3]); }

	Vector4& Matrix4::x() { return (Vector4&)(this->m[0]); }
	Vector4& Matrix4::y() { return (Vector4&)(this->m[1]); }
	Vector4& Matrix4::z() { return (Vector4&)(this->m[2]); }
	Vector4& Matrix4::w() { return (Vector4&)(this->m[3]); }
}