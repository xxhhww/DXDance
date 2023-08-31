#pragma once
#include <DirectXMath.h>
#include <unordered_map>

using namespace DirectX;

namespace Math {

	using Int = int32_t;

	class Int2 : public XMINT2 {
	public:
		inline Int2() : XMINT2(0, 0) {}
		inline Int2(int32_t x, int32_t y) : XMINT2(x, y) {}

		inline bool operator<  (Int2 v) const { return (x < v.x&& y < v.y); }
		inline bool operator<= (Int2 v) const { return (x <= v.x && y <= v.y); }
		inline bool operator>  (Int2 v) const { return (x > v.x && y > v.y); }
		inline bool operator>= (Int2 v) const { return (x >= v.x && y >= v.y); }
		inline bool operator!= (Int2 v) const { return (x != v.x || y != v.y); }
		inline bool operator== (Int2 v) const { return (x == v.x && y == v.y); }
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