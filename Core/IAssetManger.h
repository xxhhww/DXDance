#pragma once
#include <unordered_map>
#include <string>
#include <atomic>
#include <memory>
#include "Tools/ISerializable.h"

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

		inline const auto& GetName()	const { return mName; }
		inline const auto& GetID()		const { return mID; }

	protected:
		std::string mName{ "?" };
		int64_t		mID{ -1 };
		inline static std::atomic<int64_t> smAtomicIncID = 0;
	};

	/* �ʲ�����ӿ��࣬����ģ�������IAsset������ */
	template<typename TAsset>
	class IAssetManger {
	public:
		/*
		* ���캯���������ʲ�·��
		*/
		IAssetManger(const std::string& path);
		
		/*
		* ͨ��id�ж��ʲ��Ƿ����
		*/
		bool IsRegistered(int64_t id);

		/*
		* ͨ��name�ж��ʲ��Ƿ����
		*/
		bool IsRegistered(const std::string& name);
		
		/*
		* ͨ��id��ȡ�ʲ�ָ��
		*/
		TAsset* GetResource(int64_t id);

		/*
		* ͨ��name��ȡ�ʲ�ָ��
		*/
		TAsset* GetResource(const std::string& name);

	protected:
		std::unordered_map<int64_t, std::unique_ptr<TAsset>> mAssets;
		std::string mAssetPath{ "" };
	};
}

#include "IAssetManger.inl"