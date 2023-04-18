#pragma once
#include <functional>

namespace ECS {
	/*
	* 哈希帮助类
	*/
	struct MetatypeHashHelper {
		/*
		* 编译期函数，计算模板Comp的哈希值
		*/
		template<typename Comp>
		static constexpr size_t Build();

		/*
		* 编译期函数，计算可变模板...Comps的哈希值
		*/
		template<typename ...Comps>
		static constexpr size_t BuildArray();

		/*
		* 编译期函数，返回函数签名
		*/
		template<typename Comp>
		static constexpr const char* FuncSignature();

		/*
		* 编译期函数，对字符串做哈希操作
		*/
		inline static constexpr uint64_t HashFnvla(const char* key) {
			uint64_t hash = 0xcbf29ce484222325;
			uint64_t prime = 0x100000001b3;

			int i = 0;
			while (key[i]) {
				uint8_t value = key[i++];
				hash = hash ^ value;
				hash *= prime;
			}

			return hash;
		}
	};

	// 组件反射类
	struct Metatype {
	public:
		using Function = void(void*);

	public:
		/*
		* 编译期函数，为组件创建反射类
		*/
		template<typename Comp>
		static constexpr Metatype Build();

		/*
		* 编译期函数，计算参数包的总字节大小
		*/
		template<typename ...Comps>
		static constexpr size_t CalByteSize();

	public:
		// 哈希值
		size_t		hash{ 0u };
		// 内存对齐大小
		size_t		align{ 0u };
		// 内存存储大小
		size_t		size{ 0u };
		// 构造函数
		Function* constructor{ nullptr };
		// 析构函数
		Function* destructor{ nullptr };
	};
}

#include "Metatype.inl"