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
		~Scene() = default;

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
		* ���л�Ϊ����������
		*/
		void SerializeBinary(Tool::OutputMemoryStream& blob) const override;

		/*
		* �����л�����������
		*/
		void DeserializeBinary(Tool::InputMemoryStream& blob) override;

		/*
		* ���л�ΪJson����
		*/
		void SerializeJson(rapidjson::Document& doc) const override;

		/*
		* �����л�Json����
		*/
		void DeserializeJson(const rapidjson::Document& doc) override;

	private:
		int64_t								mActorIncID;	// ��������ID
		std::vector<std::unique_ptr<Actor>> mActors;		// ��������
	};
}