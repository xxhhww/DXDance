#pragma once
#pragma once
#include "Vector.h"

namespace Math {
	struct Color : public XMFLOAT3 {
	public:
		inline Color() = default;
		inline Color(float xyz) : XMFLOAT3(xyz, xyz, xyz) {}
		inline Color(float x, float y, float z) : XMFLOAT3(x, y, z) {}
		inline Color(const float* pData) : XMFLOAT3(pData) {}
		inline Color(const XMVECTOR& vec) { XMStoreFloat3(this, vec); }
		inline Color(XMVECTOR&& vec) { XMStoreFloat3(this, std::move(vec)); }
		inline Color(const Vector2& vec2, float z = 0.0f) : XMFLOAT3(vec2.x, vec2.y, z) {}
		inline Color(const Vector3& vec3) : XMFLOAT3(vec3.x, vec3.y, vec3.z) {}
		inline Color(const Vector4& vec4) : XMFLOAT3(vec4.x, vec4.y, vec4.z) {}

		inline operator XMVECTOR() const { return XMLoadFloat3(this); }
		inline operator int() const { return int(this->x); }
		inline operator float() const { return this->x; }
		inline operator bool() const { return this->x; }

		inline Color operator- () const { return XMVectorNegate(*this); }
		inline Color operator+ (Color v2) const { return XMVectorAdd(*this, v2); }
		inline Color operator- (Color v2) const { return XMVectorSubtract(*this, v2); }
		inline Color operator* (Color v2) const { return XMVectorMultiply(*this, v2); }
		inline Color operator/ (Color v2) const { return XMVectorDivide(*this, v2); }
		inline bool operator !=(Color v) const { return Vector3(XMVectorNotEqual(*this, v)).x; }
		inline bool operator ==(Color v) const { return Vector3(XMVectorEqual(*this, v)).x; }
		inline Color operator+ (float  v2) const { return *this * Color(v2); }
		inline Color operator- (float  v2) const { return *this * Color(v2); }
		inline Color operator* (float  v2) const { return *this * Color(v2); }
		inline Color operator/ (float  v2) const { return *this / Color(v2); }

		inline Color& operator += (Color v) { *this = *this + v; return *this; }
		inline Color& operator -= (Color v) { *this = *this - v; return *this; }
		inline Color& operator *= (Color v) { *this = *this * v; return *this; }
		inline Color& operator /= (Color v) { *this = *this / v; return *this; }

		inline friend Color operator* (float   v1, Color v2) { return Color(v1) * v2; }
		inline friend Color operator/ (float   v1, Color v2) { return Color(v1) / v2; }
	};
}