#pragma once
#include <unordered_map>
#include <string>
#include <memory>

namespace Core {
	/*
	* �ʲ�����ӿ��࣬����ģ�������IAsset������ 
	* ���༰�����ಢ�����ⲿ�����ʲ����ʲ��Ľ���������AssetLoader�����
	*/
	template<typename TAsset>
	class IAssetManger {
	public:
		/*
		* ���캯���������ʲ�·��
		*/
		IAssetManger(const std::string& assetPath, const std::string& enginePath);

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

		/*
		* ͨ���û��Ĳ�����ע����Դ
		*/
		virtual void RegisterResource(TAsset* target) = 0;

		/*
		* ͨ��ָ����·��(��������Ŀ·����������·��)��������������Դ.
		* ����·������Դ��AssetLoader��������ͨ��RegisterResource()������ע���������.
		*/
		virtual TAsset* LoadResource(const std::string& path) = 0;
	protected:
		std::unordered_map<int64_t, std::unique_ptr<TAsset>> mAssets;
		std::string mAssetPath{ "" };
		std::string mEnginePath{ "" };
	};
}

#include "IAssetManger.inl"