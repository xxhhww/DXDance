#pragma once
#include "Tools/ISerializable.h"
#include <atomic>

namespace Core {
	/* �ʲ��ӿ��� */
	class IAsset : public Tool::ISerializable {
	public:
		/*
		* Ĭ�Ϲ��캯���������ʲ����ļ��ж�ȡ���龰
		*/
		IAsset() = default;

		/*
		* ���캯���������ʲ��ڱ༭������ʱ�������龰����Ҫ�ṩ�ʲ�����
		*/
		inline IAsset(const std::string& name) : mName(name), mID(smAtomicIncID++) {}

		/*
		* Ĭ��������
		*/
		virtual ~IAsset() = default;

		inline void SetName(const std::string& name) { mName = name; }

		inline const auto& GetName()	const { return mName; }
		inline const auto& GetID()		const { return mID; }

	protected:
		std::string mName{ "?" };
		int64_t		mID{ -1 };
		inline static std::atomic<int64_t> smAtomicIncID = 0;
	};
}