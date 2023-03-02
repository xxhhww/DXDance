#pragma once
#include <functional>

namespace Core {
	/*
	* ��ϣ������
	*/
	struct MetatypeHashHelper {
		/*
		* �����ں���������ģ��Comp�Ĺ�ϣֵ
		*/
		template<typename Comp>
		static constexpr size_t Build();

		/*
		* �����ں���������ɱ�ģ��...Comps�Ĺ�ϣֵ
		*/
		template<typename ...Comps>
		static constexpr size_t BuildArray();

		/*
		* �����ں��������غ���ǩ��
		*/
		template<typename Comp>
		static constexpr const char* FuncSignature();

		/*
		* �����ں��������ַ�������ϣ����
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
	* ��������࣬��¼���������Ϊͨ�õ���Ϣ�뷽��
	*/
	struct Metatype {
	public:
		/*
		* �����ں�������ģ��Comp������
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
		* �����ں������������������������ֽڴ�С
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
		size_t hash{ 0u };		// �����ϣֵ
		size_t align{ 0u };		// ����ֽڶ���
		size_t size{ 0u };		// ����ֽڴ�С
		std::function<void(void*)> constructor;	// ������췽��(��void*ʹ��placement new��ʽ���й���)
		std::function<void(void*)> destructor;	// �����������(��void*ʹ���������������)
	};
}

#include "Metatype.inl"