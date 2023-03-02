#pragma once
#include <functional>

namespace Core {
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
		inline constexpr uint64_t HashFnvla(const char* key) {
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

	/*
	* 组件反射类，记录所有组件最为通用的信息与方法
	*/
	struct Metatype {
	public:
		/*
		* 编译期函数，对模板Comp做反射
		*/
		template<typename Comp>
		static constexpr Metatype build() {
			Metatype metatype{};
			metatype.hash	= MetatypeHashHelper::build<Comp>();
			metatype.align	= alignof(Comp);
			metatype.size	= sizeof(Comp);
			metatype.constructor = [](void* ptr) {
				// placement new
				new(ptr)T();
			};
			metatype.destructor	 = [](void* ptr) {
				reinterpret_cast<T*>(ptr)->~T();
			};
			return metatype;
		}

		/*
		* 编译期函数，计算参数包中组件的总字节大小
		*/
		template<typename ...Comps>
		static constexpr size_t CalByteSize() {
			const size_t sizeArray[] = { Metatype::build<Comps>().size... };
			size_t nums = sizeof(sizeArray) / sizeof(size_t);
			size_t result = 0u;
			for (size_t i = 0; i < nums; i++) {
				result += sizeArray[i];
			}
			return result;
		}
	public:
		size_t hash{ 0u };		// 组件哈希值
		size_t align{ 0u };		// 组件字节对齐
		size_t size{ 0u };		// 组件字节大小
		std::function<void(void*)> constructor;	// 组件构造方法(对void*使用placement new方式进行构造)
		std::function<void(void*)> destructor;	// 组件析构方法(对void*使用组件的析构函数)
	};
}

#include "Metatype.inl"