#pragma once
#include <functional>

namespace ECS {
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

	// ���������
	struct Metatype {
	public:
		using Function = void(void*);

	public:
		/*
		* �����ں�����Ϊ�������������
		*/
		template<typename Comp>
		static constexpr Metatype Build();

		/*
		* �����ں�������������������ֽڴ�С
		*/
		template<typename ...Comps>
		static constexpr size_t CalByteSize();

	public:
		// ��ϣֵ
		size_t		hash{ 0u };
		// �ڴ�����С
		size_t		align{ 0u };
		// �ڴ�洢��С
		size_t		size{ 0u };
		// ���캯��
		Function* constructor{ nullptr };
		// ��������
		Function* destructor{ nullptr };
	};
}

#include "Metatype.inl"