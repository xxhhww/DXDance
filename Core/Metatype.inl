#pragma once
#include "Metatype.h"

namespace Core {
	template<typename Comp>
	static constexpr size_t MetatypeHashHelper::Build() {
		// 提取原生类型
		using sanitizedType = std::remove_const_t<std::remove_reference_t<Comp>>;
		return HashFnvla(FuncSignature<sanitizedType>());
	}

	template<typename ...Comps>
	static constexpr size_t MetatypeHashHelper::BuildArray() {
		size_t hashArray[] = { Build<Comps>()... };
		constexpr uint32_t nums = sizeof(hashArray) / sizeof(size_t);
		size_t result = 0u;
		for (uint32_t i = 0; i < nums; i++) {
			result ^= hashArray[i];
		}
		return result;
	}

	template<typename Comp>
	static constexpr const char* MetatypeHashHelper::FuncSignature() {
		return __FUNCSIG__;
	}

	template<typename Comp>
	static constexpr Metatype Metatype::Build() {
		Metatype metatype{};
		metatype.hash = MetatypeHashHelper::Build<Comp>();
		metatype.align = alignof(Comp);
		metatype.size = sizeof(Comp);
		metatype.constructor = [](void* ptr) {
			// placement new
			new(ptr)Comp();
		};
		metatype.destructor = [](void* ptr) {
			reinterpret_cast<Comp*>(ptr)->~Comp();
		};
		return metatype;
	}

	template<typename ...Comps>
	static constexpr size_t Metatype::CalByteSize() {
		const size_t sizeArray[] = { Metatype::Build<Comps>().size... };
		size_t nums = sizeof(sizeArray) / sizeof(size_t);
		size_t result = 0u;
		for (size_t i = 0; i < nums; i++) {
			result += sizeArray[i];
		}
		return result;
	}
}