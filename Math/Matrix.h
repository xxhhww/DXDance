#pragma once
#include <DirectXMath.h>
#include <xutility>

using namespace DirectX;

namespace Math {
	struct Vector3;
	struct Vector4;
	struct Quaternion;

	__declspec(align(16)) struct Matrix4 : public XMFLOAT4X4 {
	public:
		inline Matrix4() { XMStoreFloat4x4(this, XMMatrixIdentity()); }
		inline Matrix4(const float* pData) : XMFLOAT4X4(pData) {}
		inline Matrix4(const XMMATRIX& matrix) { XMStoreFloat4x4(this, matrix); }
		inline Matrix4(XMMATRIX&& matrix) { XMStoreFloat4x4(this, std::move(matrix)); }
		Matrix4(const Vector3& translation, const Quaternion& rotation, const Vector3& scaling);

		Vector4 x() const;
		Vector4 y() const;
		Vector4 z() const;
		Vector4 w() const;
		Vector4& x();
		Vector4& y();
		Vector4& z();
		Vector4& w();

		inline Matrix4 Transpose() const { return XMMatrixTranspose(*this); }
		inline Matrix4 Inverse() const { return XMMatrixInverse(nullptr, *this); }

		inline operator XMMATRIX() const { return XMLoadFloat4x4(this); }

		inline Matrix4 operator* (const Matrix4& mat) const { return XMMATRIX(*this) * XMMATRIX(mat); }
		inline Matrix4& operator*= (const Matrix4& mat) { *this = *this * mat; return *this; }
	};
}