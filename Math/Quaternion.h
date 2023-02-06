#pragma once
#include <DirectXMath.h>
#include <xutility>

using namespace DirectX;

namespace Math {
	struct Vector3;
	struct Vector4;
	struct Matrix4;

	struct Quaternion : public XMFLOAT4 {
	public:
		inline Quaternion() { XMStoreFloat4(this, XMQuaternionIdentity()); }
		inline Quaternion(float xyzw) : XMFLOAT4(xyzw, xyzw, xyzw, xyzw) {}
		inline Quaternion(float x, float y, float z, float w) : XMFLOAT4(x, y, z, w) {}
		inline Quaternion(const float* pData) : XMFLOAT4(pData) {}
		inline Quaternion(const XMVECTOR& vec) { XMStoreFloat4(this, vec); }
		inline Quaternion(XMVECTOR&& vec) { XMStoreFloat4(this, std::move(vec)); }
		// vec3作欧拉角，转换为四元数
		Quaternion(const Vector3& vec3);
		Quaternion(const Vector4& vec4);

		// 将四元数转换为欧拉角
		Vector3 EulerAngle() const;
		Vector3 VecXyz() const;
		Vector3 VecXzy() const;
		Vector3 VecYxz() const;
		Vector3 VecYzx() const;
		Vector3 VecZxy() const;
		Vector3 VecZyx() const;
		// 将四元数转换为矩阵
		Matrix4 RotationMatrix() const;

		inline Quaternion Normalize() const { return XMQuaternionNormalize(*this); }
		inline Quaternion Length() const { return XMQuaternionLength(*this); }
		inline Quaternion LengthSq() const { return XMQuaternionLengthSq(*this); }

		inline operator XMVECTOR() const { return XMLoadFloat4(this); }
	};
}