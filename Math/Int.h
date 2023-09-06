#pragma once
#include <DirectXMath.h>
#include <unordered_map>

using namespace DirectX;

namespace Math {

	using Int = int32_t;

	class Int2 : public XMINT2 {
	public:
		inline Int2() : XMINT2(0, 0) {}
		inline Int2(float xy) : XMINT2(xy, xy) {}
		inline Int2(int32_t x, int32_t y) : XMINT2(x, y) {}
		inline Int2(const XMVECTOR& vec) { XMStoreSInt2(this, vec); }
		inline Int2(XMVECTOR&& vec) { XMStoreSInt2(this, std::move(vec)); }

		inline operator XMVECTOR() const { return XMLoadSInt2(this); }

		inline bool operator<  (Int2 v) const { return (x < v.x&& y < v.y); }
		inline bool operator<= (Int2 v) const { return (x <= v.x && y <= v.y); }
		inline bool operator>  (Int2 v) const { return (x > v.x && y > v.y); }
		inline bool operator>= (Int2 v) const { return (x >= v.x && y >= v.y); }
		inline bool operator!= (Int2 v) const { return (x != v.x || y != v.y); }
		inline bool operator== (Int2 v) const { return (x == v.x && y == v.y); }

		inline Int2 operator- () const { return XMVectorNegate(*this); }

		inline Int2 operator+ (Int2 v) const { return XMVectorAdd(*this, v); }
		inline Int2 operator- (Int2 v) const { return XMVectorSubtract(*this, v); }
		inline Int2 operator* (Int2 v) const { return XMVectorMultiply(*this, v); }
		inline Int2 operator/ (Int2 v) const { return XMVectorDivide(*this, v); }
		inline Int2 operator+ (int32_t  v) const { return *this * Int2(v); }
		inline Int2 operator- (int32_t  v) const { return *this * Int2(v); }
		inline Int2 operator* (int32_t  v) const { return *this * Int2(v); }
		inline Int2 operator/ (int32_t  v) const { return *this / Int2(v); }

		inline Int2& operator += (Int2 v) { *this = *this + v; return *this; }
		inline Int2& operator -= (Int2 v) { *this = *this - v; return *this; }
		inline Int2& operator *= (Int2 v) { *this = *this * v; return *this; }
		inline Int2& operator /= (Int2 v) { *this = *this / v; return *this; }

		inline friend Int2 operator* (int32_t   v1, Int2 v2) { return Int2(v1) * v2; }
		inline friend Int2 operator/ (int32_t   v1, Int2 v2) { return Int2(v1) / v2; }
	};

	struct HashInt2 {
	public:
		size_t operator()(const Math::Int2& v) const {
			return std::hash<int32_t>()(v.x) ^ std::hash<int32_t>()(v.y);
		}
	};

	class Int3 : public XMINT3 {
	public:
		inline Int3() : XMINT3(0, 0, 0) {}
		inline Int3(int32_t x, int32_t y, int32_t z) : XMINT3(x, y, z) {}

		inline bool operator<  (Int3 v) const { return (x < v.x&& y < v.y&& z < v.z); }
		inline bool operator<= (Int3 v) const { return (x <= v.x && y <= v.y && z <= v.z); }
		inline bool operator>  (Int3 v) const { return (x > v.x && y > v.y && z > v.z); }
		inline bool operator>= (Int3 v) const { return (x >= v.x && y >= v.y && z >= v.z); }
		inline bool operator!= (Int3 v) const { return (x != v.x || y != v.y || z != v.z); }
		inline bool operator== (Int3 v) const { return (x == v.x && y == v.y && z == v.z); }
	};

	class Int4 : public XMINT4 {
	public:
		inline Int4() : XMINT4(0, 0, 0, 0) {}
		inline Int4(int32_t x, int32_t y, int32_t z, int32_t w) : XMINT4(x, y, z, w) {}

		inline bool operator<  (Int4 v) const { return (x < v.x&& y < v.y&& z < v.z&& w < v.w); }
		inline bool operator<= (Int4 v) const { return (x <= v.x && y <= v.y && z <= v.z && w <= v.w); }
		inline bool operator>  (Int4 v) const { return (x > v.x && y > v.y && z > v.z && w > v.w); }
		inline bool operator>= (Int4 v) const { return (x >= v.x && y >= v.y && z >= v.z && w >= v.w); }
		inline bool operator!= (Int4 v) const { return (x != v.x || y != v.y || z != v.z || w != v.w); }
		inline bool operator== (Int4 v) const { return (x == v.x && y == v.y && z == v.z && w == v.w); }
	};

}