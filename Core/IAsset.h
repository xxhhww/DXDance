#pragma once
#include "Tools/ISerializable.h"
#include <atomic>

namespace Core {
	/* �ʲ��ӿ��� */
	class IAsset : public Tool::ISerializable {
	public:
		/*
		* Ĭ�Ϲ��캯��
		*/
		IAsset() = default;

		/*
		* Ĭ��������
		*/
		virtual ~IAsset() = default;

		/*
		* Set����
		*/
		inline void SetName	(const std::string& name)	{ mName = name; }
		inline void SetUID	(int64_t id)				{ mID = id; }

		/*
		* Get����
		*/
		inline const auto& GetName()	const { return mName; }
		inline const auto& GetUID()		const { return mID; }

	protected:
		std::string mName{ "" };
		int64_t		mID{ -1 };
	};
}