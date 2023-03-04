#pragma once
#include "IAsset.h"
#include "Actor.h"

namespace Core {

	/*
	* Scene(����)���ʲ�����Ķ�������������������ó�����ص�Actor(�������壬Ҳ������ECS���)
	*/
	class Scene : public IAsset {
	public:
		/*
		* ���캯��
		*/
		Scene() = default;

		/*
		* ��������
		*/
		~Scene() {
			int i = 32;
		}

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
		void SerializeJson(Tool::JsonWriter& writer) const override;

		/*
		* �����л�Json����
		*/
		void DeserializeJson(const Tool::JsonReader& reader) override;

	private:
		int64_t								mActorIncID{ 0 };	// ��������ID
		std::vector<std::unique_ptr<Actor>> mActors;			// ��������
	};
}