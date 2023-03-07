#pragma once
#include "Tools/JsonHelper.h"
#include <atomic>
#include <string>

namespace Core {
	/*
	* �ʲ�״̬
	*/
	enum class AssetStatus {
		UnLoad,	// δ����
		Loaded	// �Ѽ���
	};

	/* �ʲ��ӿ��࣬�ʲ����ڴ�������ʲ������������ */
	class IAsset {
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
		* ����
		*/
		virtual void Load(bool aSync = false) = 0;

		/*
		* ж��
		*/
		virtual void Unload() = 0;

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
		inline void SetPath	(const std::string& path)	{ mPath = path; }
		inline void SetUID	(int64_t id)				{ mID = id; }

		/*
		* Get����
		*/
		inline const auto& GetPath()	const { return mPath; }
		inline const auto& GetUID()		const { return mID; }
		inline const auto& GetStatus()	const { return mStatus; }

	protected:
		std::string		mPath;					// �ʲ�·��
		int64_t			mID{ -1 };				// �ʲ�ID
		std::atomic<uint32_t> mRefCount{ 1u };	// ���ü���
		AssetStatus		mStatus{ AssetStatus::UnLoad };	// �ʲ�״̬
	};
}