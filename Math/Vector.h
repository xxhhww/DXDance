#pragma once
#include <DirectXMath.h>
#include <xutility>

using namespace DirectX;

namespace Math {
	struct Color;
	struct Vector3;
	struct Vector4;
	struct Matrix4;
	struct Quaternion;

	struct Vector2 : public XMFLOAT2 {
	public:
		inline Vector2() : XMFLOAT2(0.0f, 0.0f) {}
		inline Vector2(float xy) : XMFLOAT2(xy, xy) {}
		inline Vector2(float x, float y) : XMFLOAT2(x, y) {}
		inline Vector2(const float* pData) : XMFLOAT2(pData) {}
		inline Vector2(const XMVECTOR& vec) { XMStoreFloat2(this, vec); }
		inline Vector2(XMVECTOR&& vec) { XMStoreFloat2(this, std::move(vec)); }
		Vector2(const Vector3& vec3);
		Vector2(const Vector4& vec4);
		Vector2(const Color& color);

		inline Vector2 Normalize() const { return XMVector2Normalize(*this); }
		inline float Length() const { return Vector2(XMVector2Length(*this)).x; }
		inline float LengthSq() const { return Vector2(XMVector2LengthSq(*this)).x; }
		inline float Dot(const Vector2& vec2) const { return Vector2(XMVector2Dot(*this, vec2)).x; }
		inline Vector2 Cross(const Vector2& vec2) const { return Vector2(XMVector2Cross(*this, vec2)).x; }

		inline operator XMVECTOR() const { return XMLoadFloat2(this); }
		inline operator int() const { return int(this->x); }
		inline operator float() const { return this->x; }
		inline operator bool() const { return this->x; }

		inline bool operator<  (Vector2 v) const { return (x < v.x && y < v.y);   }
		inline bool operator<= (Vector2 v) const { return (x <= v.x && y <= v.y); }
		inline bool operator>  (Vector2 v) const { return (x > v.x && y > v.y);   }
		inline bool operator>= (Vector2 v) const { return (x >= v.x && y >= v.y); }
		inline bool operator!= (Vector2 v) const { return (x != v.x || y != v.y); }
		inline bool operator== (Vector2 v) const { return (x == v.x && y == v.y); }

		inline Vector2 operator- () const { return XMVectorNegate(*this); }
		
		inline Vector2 operator+ (Vector2 v) const { return XMVectorAdd(*this, v);      }
		inline Vector2 operator- (Vector2 v) const { return XMVectorSubtract(*this, v); }
		inline Vector2 operator* (Vector2 v) const { return XMVectorMultiply(*this, v); }
		inline Vector2 operator/ (Vector2 v) const { return XMVectorDivide(*this, v);   }
		inline Vector2 operator+ (float  v) const { return *this * Vector2(v); }
		inline Vector2 operator- (float  v) const { return *this * Vector2(v); }
		inline Vector2 operator* (float  v) const { return *this * Vector2(v); }
		inline Vector2 operator/ (float  v) const { return *this / Vector2(v); }

		inline Vector2& operator += (Vector2 v) { *this = *this + v; return *this; }
		inline Vector2& operator -= (Vector2 v) { *this = *this - v; return *this; }
		inline Vector2& operator *= (Vector2 v) { *this = *this * v; return *this; }
		inline Vector2& operator /= (Vector2 v) { *this = *this / v; return *this; }

		inline friend Vector2 operator* (float   v1, Vector2 v2) { return Vector2(v1) * v2; }
		inline friend Vector2 operator/ (float   v1, Vector2 v2) { return Vector2(v1) / v2; }
	};

	struct Vector3 : public XMFLOAT3 {
	public:
		inline Vector3() :XMFLOAT3(0.0f, 0.0f, 0.0f) {}
		inline Vector3(float xyz) : XMFLOAT3(xyz, xyz, xyz) {}
		inline Vector3(float x, float y, float z) : XMFLOAT3(x, y, z) {}
		inline Vector3(const float* pData) : XMFLOAT3(pData) {}
		inline Vector3(const XMVECTOR& vec) { XMStoreFloat3(this, vec); }
		inline Vector3(XMVECTOR&& vec) { XMStoreFloat3(this, std::move(vec)); }
		Vector3(const Vector2& vec2, float z = 0.0f);
		Vector3(const Vector4& vec4);
		Vector3(const Color& color);

		inline Vector3 Normalize() const { return XMVector3Normalize(*this); }
		inline float Length() const { return Vector3(XMVector3Length(*this)).x; }
		inline float LengthSq() const { return Vector3(XMVector3LengthSq(*this)).x; }
		inline float Dot(const Vector3& vec3) const { return Vector3(XMVector3Dot(*this, vec3)).x; }
		inline Vector3 Cross(const Vector3& vec3) const { return Vector3(XMVector3Cross(*this, vec3)).x; }

		Matrix4 TranslationMatrix() const;
		Matrix4 RotationMatrix() const;
		Matrix4 ScalingMatrix() const;

		// 将三维向量看作点进行矩阵变换
		Vector4 PointTransform(const Matrix4& matrix) const;
		// 将三维向量看作向量进行矩阵变换
		Vector4 NormalTransform(const Matrix4& matrix) const;

		inline operator XMVECTOR() const { return XMLoadFloat3(this); }
		inline operator int() const { return int(this->x); }
		inline operator float() const { return this->x; }
		inline operator bool() const { return this->x; }

		inline bool operator<  (Vector3 v) const { return (x < v.x && y < v.y && z < v.z);    }
		inline bool operator<= (Vector3 v) const { return (x <= v.x && y <= v.y && z <= v.z); }
		inline bool operator>  (Vector3 v) const { return (x > v.x && y > v.y && z > v.z);    }
		inline bool operator>= (Vector3 v) const { return (x >= v.x && y >= v.y && z >= v.z); }
		inline bool operator!= (Vector3 v) const { return (x != v.x || y != v.y || z != v.z); }
		inline bool operator== (Vector3 v) const { return (x == v.x && y == v.y && z == v.z); }

		inline Vector3 operator- () const { return XMVectorNegate(*this); }
		
		inline Vector3 operator+ (Vector3 v) const { return XMVectorAdd(*this, v);      }
		inline Vector3 operator- (Vector3 v) const { return XMVectorSubtract(*this, v); }
		inline Vector3 operator* (Vector3 v) const { return XMVectorMultiply(*this, v); }
		inline Vector3 operator/ (Vector3 v) const { return XMVectorDivide(*this, v);   }
		inline Vector3 operator+ (float  v) const { return *this * Vector3(v); }
		inline Vector3 operator- (float  v) const { return *this * Vector3(v); }
		inline Vector3 operator* (float  v) const { return *this * Vector3(v); }
		inline Vector3 operator/ (float  v) const { return *this / Vector3(v); }

		inline Vector3& operator += (Vector3 v) { *this = *this + v; return *this; }
		inline Vector3& operator -= (Vector3 v) { *this = *this - v; return *this; }
		inline Vector3& operator *= (Vector3 v) { *this = *this * v; return *this; }
		inline Vector3& operator /= (Vector3 v) { *this = *this / v; return *this; }

		inline friend Vector3 operator* (float   v1, Vector3 v2) { return Vector3(v1) * v2; }
		inline friend Vector3 operator/ (float   v1, Vector3 v2) { return Vector3(v1) / v2; }
	};

	struct Vector4 : public XMFLOAT4 {
	public:
		inline Vector4() : XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f) {}
		inline Vector4(float xyzw) : XMFLOAT4(xyzw, xyzw, xyzw, xyzw) {}
		inline Vector4(float x, float y, float z, float w) : XMFLOAT4(x, y, z, w) {}
		inline Vector4(const float* pData) : XMFLOAT4(pData) {}
		inline Vector4(const XMVECTOR& Vec) { XMStoreFloat4(this, Vec); }
		inline Vector4(XMVECTOR&& vec) { XMStoreFloat4(this, std::move(vec)); }
		Vector4(const Vector2& vec2, float z = 0.0f, float w = 0.0f);
		Vector4(const Vector3& vec3, float w = 0.0f);
		Vector4(const Color& color, float w = 0.0f);

		inline Vector4 Normalize() const { return XMVector4Normalize(*this); }
		inline float Length() const { return Vector4(XMVector4Length(*this)).x; }
		inline float LengthSq() const { return Vector4(XMVector4LengthSq(*this)).x; }
		inline float Dot(const Vector4& vec4) const { return Vector4(XMVector4Dot(*this, vec4)).x; }

		inline operator XMVECTOR() const { return XMLoadFloat4(this); }
		inline operator int() const { return int(this->x); }
		inline operator float() const { return this->x; }
		inline operator bool() const { return this->x; }

		inline bool operator<  (Vector4 v) const { return (x < v.x && y < v.y && z < v.z && w < v.w); }
		inline bool operator<= (Vector4 v) const { return (x <= v.x && y <= v.y && z <= v.z && w <= v.w); }
		inline bool operator>  (Vector4 v) const { return (x > v.x && y > v.y && z > v.z && w > v.w); }
		inline bool operator>= (Vector4 v) const { return (x >= v.x && y >= v.y && z >= v.z && w >= v.w); }
		inline bool operator!= (Vector4 v) const { return (x != v.x || y != v.y || z != v.z || w != v.w); }
		inline bool operator== (Vector4 v) const { return (x == v.x && y == v.y && z == v.z && w == v.w); }
		
		inline Vector4 operator- () const { return Vector4(XMVectorNegate(*this)); }
		
		inline Vector4 operator+ (Vector4 v) const { return Vector4(XMVectorAdd(*this, v));      }
		inline Vector4 operator- (Vector4 v) const { return Vector4(XMVectorSubtract(*this, v)); }
		inline Vector4 operator* (Vector4 v) const { return Vector4(XMVectorMultiply(*this, v)); }
		inline Vector4 operator/ (Vector4 v) const { return Vector4(XMVectorDivide(*this, v));   }
		inline Vector4 operator+ (float  v) const { return *this * Vector4(v); }
		inline Vector4 operator- (float  v) const { return *this * Vector4(v); }
		inline Vector4 operator* (float  v) const { return *this * Vector4(v); }
		inline Vector4 operator/ (float  v) const { return *this / Vector4(v); }

		inline Vector4& operator += (Vector4 v) { *this = *this + v; return *this; }
		inline Vector4& operator -= (Vector4 v) { *this = *this - v; return *this; }
		inline Vector4& operator *= (Vector4 v) { *this = *this * v; return *this; }
		inline Vector4& operator /= (Vector4 v) { *this = *this / v; return *this; }

		Vector4 operator* (const Matrix4& mat);

		inline friend Vector4 operator* (float   v1, Vector4 v2) { return Vector4(v1) * v2; }
		inline friend Vector4 operator/ (float   v1, Vector4 v2) { return Vector4(v1) / v2; }
	};


#define CREATE_SIMD_FUNCTIONS( TYPE ) \
	inline TYPE Sqrt( const TYPE& s ) { return TYPE(XMVectorSqrt(s)); } \
	inline TYPE Recip( const TYPE& s ) { return TYPE(XMVectorReciprocal(s)); } \
	inline TYPE RecipSqrt( const TYPE& s ) { return TYPE(XMVectorReciprocalSqrt(s)); } \
	inline TYPE Floor( const TYPE& s ) { return TYPE(XMVectorFloor(s)); } \
	inline TYPE Ceiling( const TYPE& s ) { return TYPE(XMVectorCeiling(s)); } \
	inline TYPE Round( const TYPE& s ) { return TYPE(XMVectorRound(s)); } \
	inline TYPE Abs( const TYPE& s ) { return TYPE(XMVectorAbs(s)); } \
	inline TYPE Exp( const TYPE& s ) { return TYPE(XMVectorExp(s)); } \
	inline TYPE Pow( const TYPE& b, const TYPE& e ) { return TYPE(XMVectorPow(b, e)); } \
	inline TYPE Log( const TYPE& s ) { return TYPE(XMVectorLog(s)); } \
	inline TYPE Sin( const TYPE& s ) { return TYPE(XMVectorSin(s)); } \
	inline TYPE Cos( const TYPE& s ) { return TYPE(XMVectorCos(s)); } \
	inline TYPE Tan( const TYPE& s ) { return TYPE(XMVectorTan(s)); } \
	inline TYPE ASin( const TYPE& s ) { return TYPE(XMVectorASin(s)); } \
	inline TYPE ACos( const TYPE& s ) { return TYPE(XMVectorACos(s)); } \
	inline TYPE ATan( const TYPE& s ) { return TYPE(XMVectorATan(s)); } \
	inline TYPE ATan2( const TYPE& y, const TYPE& x ) { return TYPE(XMVectorATan2(y, x)); } \
	inline TYPE Lerp( const TYPE& a, const TYPE& b, const TYPE& t ) { return TYPE(XMVectorLerpV(a, b, t)); } \
    inline TYPE Lerp( const TYPE& a, const TYPE& b, float t ) { return TYPE(XMVectorLerp(a, b, t)); } \
	inline TYPE Max( const TYPE& a, const TYPE& b ) { return TYPE(XMVectorMax(a, b)); } \
	inline TYPE Min( const TYPE& a, const TYPE& b ) { return TYPE(XMVectorMin(a, b)); } \
	inline TYPE Clamp( const TYPE& v, const TYPE& a, const TYPE& b ) { return Min(Max(v, a), b); } \

	CREATE_SIMD_FUNCTIONS(Vector2);
	CREATE_SIMD_FUNCTIONS(Vector3);
	CREATE_SIMD_FUNCTIONS(Vector4);
}