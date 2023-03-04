#pragma once
#include "Entity.h"
#include "Tools/Event.h"
#include "Tools/ISerializable.h"

namespace Core {
	/*
	* ��������
	*/
	class Actor : public Tool::ISerializable {
	public:
		/*
		* ���캯��
		*/
		Actor(int64_t actorID, const std::string& name);

		/*
		* ��������
		*/
		~Actor();

		/*
		* ������
		*/
		template<typename Comp>
		Comp& AddComponent();

		/*
		* ������
		*/
		template<typename Comp>
		Comp& AddComponent(Comp& comp);

		/*
		* ������
		*/
		template<typename Comp>
		Comp& GetComponent();

		/*
		* ɾ�����
		*/
		template<typename Comp>
		void DelComponent();

		/*
		* ӵ�����
		*/
		template<typename Comp>
		bool HasComponent() const;

		/*
		* ��Ӹ��ڵ�
		*/
		void AttachParent(Actor& parent);

		/*
		* �Ƴ����ڵ�
		*/
		void DetachParent();

		/*
		* ��������
		*/
		void SetName(const std::string& name);

		/*
		* ���ü���״̬�����󱻼���ʱ���Ὣ������Ҳ����Ϊ����״̬������ʧ��ʱ�򲻻�
		*/
		void SetActive(bool isActive);

		/*
		* ��������״̬����������ʱ���Ὣ�Ӷ���Ҳ����Ϊ����״̬
		*/
		void Destory();

		/*
		* Get����
		*/
		inline const auto& GetName()		const { return mName; }
		inline const auto& GetID()			const { return mActorID; }
		inline const auto& GetActive()		const { return mActive; }
		inline const auto& GetDestoryed()	const { return mDestoryed; }
		inline const auto& GetChilds()		const { return mChilds; }
		inline const auto& GetParentID()	const { return mParentID; }
		inline Actor*	   GetParent()		const { return mParent; }
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
		int64_t				mActorID{ -1 };		// ����ID(��Entity��ID��ͬ��ActorID��Ҫ�־û��洢)
		std::string			mName;				// ��������(��ʾ��Hirerachy��)

		bool				mActive{ true };	// �Ƿ񼤻�
		bool				mDestoryed{ false };// �Ƿ�����

		int64_t				mParentID{ -1 };	// ������ID
		Actor*				mParent{ nullptr };	// ������
		std::vector<Actor*> mChilds;			// ������

		Entity				mEntity;			// ECSʵ��
	};
}

#include "Actor.inl"