#pragma once
#include "Actor.h"

#include "ECS/CCamera.h"
#include "ECS/CTransform.h"

namespace Core {

	class WorldManger;

	class World {
		friend class WorldManger;
	public:
		World(WorldManger* manger);
		~World() = default;

		/*
		* ����Actor(�ṩ������Ӧ���Ƕ��ص�)
		*/
		Actor* CreateActor(const std::string& name);

		/*
		* ɾ��Actor
		*/
		void DeleteActor(Actor* actor);

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

	public:
		ECS::Camera	   editorCamera;
		ECS::Transform editorTransform;

	private:
		WorldManger* mManger{ nullptr };				// �ʲ�������
		int64_t	mActorIncID{ 0 };						// ��������ID
		std::vector<std::unique_ptr<Actor>> mActors;	// ��������

	};

}