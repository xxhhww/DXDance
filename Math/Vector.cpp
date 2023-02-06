#include "Vector.h"
#include "Quaternion.h"
#include "Matrix.h"
#include "Color.h"

namespace Math {
	Vector2::Vector2(const Vector3& vec3) : XMFLOAT2(vec3.x, vec3.y) {}
	Vector2::Vector2(const Vector4& vec4) : XMFLOAT2(vec4.x, vec4.y) {}
	Vector2::Vector2(const Color& color) : XMFLOAT2(color.x, color.y) {}

	Vector3::Vector3(const Vector2& vec2, float z) : XMFLOAT3(vec2.x, vec2.y, z) {}
	Vector3::Vector3(const Vector4& vec4) : XMFLOAT3(vec4.x, vec4.y, vec4.z) {}
	Vector3::Vector3(const Color& color) : XMFLOAT3(color.x, color.y, color.z) {}

	Vector4::Vector4(const Vector2& vec2, float z, float w) : XMFLOAT4(vec2.x, vec2.y, z, w) {}
	Vector4::Vector4(const Vector3& vec3, float w) : XMFLOAT4(vec3.x, vec3.y, vec3.z, w) {}
	Vector4::Vector4(const Color& color, float w) : XMFLOAT4(color.x, color.y, color.z, w) {}

	Matrix4 Vector3::TranslationMatrix() const { return XMMatrixTranslation(x, y, z); }
	Matrix4 Vector3::RotationMatrix() const { return XMMatrixRotationRollPitchYaw(x, y, z); }
	Matrix4 Vector3::ScalingMatrix() const { return XMMatrixScaling(x, y, z); }

	Vector4 Vector3::PointTransform(const Matrix4& matrix) const { return Vector4(*this, 1.0f) * matrix; }
	Vector4 Vector3::NormalTransform(const Matrix4& matrix) const { return Vector4(*this, 0.0f) * matrix; }

	Vector4 Vector4::operator*(const Matrix4& mat) { return XMVector4Transform(*this, mat); }
}