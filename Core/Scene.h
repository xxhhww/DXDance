#pragma once
#include "Actor.h"

#include "ECS/CCamera.h"
#include "ECS/CTransform.h"

namespace Core {

	class SceneManger;

	/*
	* Scene���ʲ�����Ķ�������������������ó�����ص�Actor
	*/
	class Scene {
		friend class SceneManger;
	public:
		Scene(SceneManger* manger, int64_t uid);
		~Scene() = default;

		/*
		* ����
		*/
		void Load();

		/*
		* ж��
		*/
		void Unload();

		/*
		* ���浽����
		*/
		void SaveToDisk();

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
		inline const auto& GetUID() const { return mUID; }

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
		// Scene�����˵�ʲ����в���Ÿö����·�������Ǵ���ʲ�UID
		// ��Ϊ�����·���ǿɱ�ģ������ڱ༭�������·���任ʱ�޸ķ�����е��ʲ�·������ 
		// ���ʲ�������ڲ���Ҫʹ�øö����·��ʱ����ѯ�ʲ�·������

		int64_t								mUID{ 0 };			// �ʲ�UID
		SceneManger*						mManger{ nullptr };	// �ʲ�������

		int64_t								mActorIncID{ 0 };	// ��������ID
		std::vector<std::unique_ptr<Actor>> mActors;			// ��������
	};

}