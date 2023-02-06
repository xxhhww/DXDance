#include "Quaternion.h"
#include "Vector.h"
#include "Matrix.h"
#include <cmath>

namespace Math {
#define CLAMP(x , min , max) ((x) > (max) ? (max) : ((x) < (min) ? (min) : x))

	Quaternion::Quaternion(const Vector3& vec3) {
		XMStoreFloat4(this, XMQuaternionRotationRollPitchYaw(vec3.x, vec3.y, vec3.z));
	}

	Quaternion::Quaternion(const Vector4& vec4)
		: XMFLOAT4(vec4.x, vec4.y, vec4.z, vec4.w) {
	}

	Vector3 Quaternion::EulerAngle() const {
		Vector3 eulerAngle;
		eulerAngle.z = atan2(2.0f * (x * y + z * w), 1.0f - 2.0f * (x * x + z * z));
		eulerAngle.x = asin(CLAMP(2.0f * (x * w - y * z), -1.0f, 1.0f));
		eulerAngle.y = atan2(2.0f * (y * w + x * z), 1.0f - 2.0f * (y * y + x * x));
		return eulerAngle;
	}

	Matrix4 Quaternion::RotationMatrix() const { return XMMatrixRotationQuaternion(*this); }

	Vector3 Quaternion::VecXyz() const
	{
		double ii = x * x;
		double jj = y * y;
		double kk = z * z;
		double ei = w * x;
		double ej = w * y;
		double ek = w * z;
		double ij = x * y;
		double ik = x * z;
		double jk = y * z;

		Vector3 result;
		result.x = (float)std::atan2(2.0f * (ei - jk), 1 - 2.0f * (ii + jj));
		result.y = (float)std::asin(2.0f * (ej + ik));
		result.z = (float)std::atan2(2.0f * (ek - ij), 1 - 2.0f * (jj + kk));
		return result;
	}

	Vector3 Quaternion::VecXzy() const
	{
		double ii = x * x;
		double jj = y * y;
		double kk = z * z;
		double ei = w * x;
		double ej = w * y;
		double ek = w * z;
		double ij = x * y;
		double ik = x * z;
		double jk = y * z;
		Vector3 result;
		result.x = (float)std::atan2(2.0f * (ei + jk), 1 - 2.0f * (ii + kk));
		result.y = (float)std::atan2(2.0f * (ej + ik), 1 - 2.0f * (jj + kk));
		result.z = (float)std::asin(2.0f * (ek - ij));
		return result;
	}

	Vector3 Quaternion::VecYxz() const
	{
		double ii = x * x;
		double jj = y * y;
		double kk = z * z;
		double ei = w * x;
		double ej = w * y;
		double ek = w * z;
		double ij = x * y;
		double ik = x * z;
		double jk = y * z;
		Vector3 result;
		result.x = (float)std::asin(2.0f * (ei - jk));
		result.y = (float)std::atan2(2.0f * (ej + ik), 1 - 2.0f * (ii + jj));
		result.z = (float)std::atan2(2.0f * (ek + ij), 1 - 2.0f * (ii + kk));
		return result;
	}

	Vector3 Quaternion::VecYzx() const
	{
		double ii = x * x;
		double jj = y * y;
		double kk = z * z;
		double ei = w * x;
		double ej = w * y;
		double ek = w * z;
		double ij = x * y;
		double ik = x * z;
		double jk = y * z;
		Vector3 result;
		result.x = (float)std::atan2(2.0f * (ei - jk), 1 - 2.0f * (ii + kk));
		result.y = (float)std::atan2(2.0f * (ej - ik), 1 - 2.0f * (jj + kk));
		result.z = (float)std::asin(2.0f * (ek + ij));
		return result;
	}

	Vector3 Quaternion::VecZxy() const
	{
		double ii = x * x;
		double jj = y * y;
		double kk = z * z;
		double ei = w * x;
		double ej = w * y;
		double ek = w * z;
		double ij = x * y;
		double ik = x * z;
		double jk = y * z;
		Vector3 result;
		result.x = (float)std::asin(2.0f * (ei + jk));
		result.y = (float)std::atan2(2.0f * (ej - ik), 1 - 2.0f * (ii + jj));
		result.z = (float)std::atan2(2.0f * (ek - ij), 1 - 2.0f * (ii + kk));
		return result;
	}

	Vector3 Quaternion::VecZyx() const
	{
		double ii = x * x;
		double jj = y * y;
		double kk = z * z;
		double ei = w * x;
		double ej = w * y;
		double ek = w * z;
		double ij = x * y;
		double ik = x * z;
		double jk = y * z;
		Vector3 result;
		result.x = (float)std::atan2(2.0f * (ei + jk), 1 - 2.0f * (ii + jj));
		result.y = (float)std::asin(2.0f * (ej - ik));
		result.z = (float)std::atan2(2.0f * (ek + ij), 1 - 2.0f * (jj + kk));
		return result;
	}

}