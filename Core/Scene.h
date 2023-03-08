#pragma once
#include "IAsset.h"
#include "IAssetManger.h"
#include "Actor.h"

namespace Core {

	/*
	* Scene���ʲ�����Ķ�������������������ó�����ص�Actor
	*/
	class Scene : public IAsset {
	public:
		/*
		* ���캯��
		*/
		Scene(IAssetManger<Scene>* manger);

		/*
		* ��������
		*/
		~Scene() = default;

		/*
		* ����
		*/
		void Load(bool aSync = false);

		/*
		* ж��
		*/
		void Unload();


		/*
		* ����Actor(�ṩ������Ӧ���Ƕ��ص�)
		*/
		Actor* CreateActor(const std::string& name);

		/*
		* ͨ��IDѰ��Actor
		*/
		Actor* FindActorByID(int32_t id);

		/*
		* ͨ������Ѱ��Actor
		*/
		Actor* FindActorByName(const std::string& name);

	public:
		/*
		* ���л�ΪJson����
		*/
		void SerializeJson(Tool::JsonWriter& writer) const;

		/*
		* �����л�Json����
		*/
		void DeserializeJson(const Tool::JsonReader& reader);

	private:
		IAssetManger<Scene>*				mManger{ nullptr };	// ������
		int64_t								mActorIncID{ 0 };	// ��������ID
		std::vector<std::unique_ptr<Actor>> mActors;			// ��������
	};
}