#pragma once
#include "Tools/JsonHelper.h"
#include <atomic>
#include <string>

namespace Core {
	class IAssetManger;

	/*
	* �ʲ�״̬
	*/
	enum class AssetStatus {
		UnLoad,	// δ����
		Loaded	// �Ѽ���
	};

	/* �ʲ��ӿ��� */
	class IAsset {
	public:
		/*
		* Ĭ�Ϲ��캯��
		*/
		IAsset(IAssetManger* assetManger);

		/*
		* Ĭ��������
		*/
		virtual ~IAsset();

		/*
		* ����
		*/
		virtual void Load(const std::string& path, bool aSync = false) = 0;

		/*
		* ж��
		*/
		virtual void UnLoad(const std::string& path) = 0;

		/*
		* ���ü�����һ
		*/
		inline uint32_t IncRefCount() { return ++mRefCount; }

		/*
		* ���ü�����һ
		*/
		inline uint32_t DecRefCount() { return --mRefCount; }

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
		inline const auto& GetStatus()	const { return mStatus; }
	protected:
		IAssetManger*	mManger{ nullptr };		// �ʲ�������
		std::string		mName{ "" };			// �ʲ�����
		int64_t			mID{ -1 };				// �ʲ�ID
		AssetStatus		mStatus;				// �ʲ�״̬
		std::atomic<uint32_t> mRefCount{ 1u };	// ���ü���
	};
}