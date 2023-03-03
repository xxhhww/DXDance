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
		Actor(const std::string& name);

		/*
		* ��������
		*/
		~Actor();

		/*
		* ��������
		*/
		void SetName(const std::string& name);

		/*
		* ��Ӹ��ڵ�
		*/
		void AttachParent(Actor& parent);

		/*
		* �Ƴ����ڵ�
		*/
		void DetachParent();

		/*
		* Get����
		*/
		inline const auto& GetName() const { return mName; }
		inline const auto& GetID()   const { return mEntity.GetID(); }
	public:
		/*
		* ���л�Ϊ����������
		*/
		void SerializeBinary(Tool::OutputMemoryStream& blob) const override {}

		/*
		* �����л�����������
		*/
		void DeserializeBinary(const Tool::InputMemoryStream& blob) override {}

		/*
		* ���л�ΪJson����
		*/
		void SerializeJson(rapidjson::Document& doc) const override;

		/*
		* �����л�Json����
		*/
		void DeserializeJson(const rapidjson::Document& doc) override;

	private:
		std::string			mName;				// ��������(��ʾ��Hirerachy��)
		Actor*				mParent{ nullptr };	// ������
		std::vector<Actor*> mChilds;			// ������

		Entity				mEntity;			// ECSʵ��
	};
}