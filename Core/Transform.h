#pragma once
#include "IComponent.h"
#include "Math/Vector.h"
#include "Math/Quaternion.h"
#include "Math/Matrix.h"

namespace Core {
	class Transform : public IComponent {
	public:
		Transform() = default;

		/*
		* ��������ڸ��ڵ��λ��
		*/
		void SetLocalPosition(const Math::Vector3& position);

		/*
		* ��������ڸ��ڵ����ת
		*/
		void SetLocalRotation(const Math::Quaternion& rotation);

		/*
		* ��������ڸ��ڵ������
		*/
		void SetLocalScale(const Math::Vector3& scale);

		/*
		* ���þ�̬����
		*/
		void SetStatic(bool isStatic);

		inline const auto& GetLocalPosition() const { return mLocalPosition; }
		inline const auto& GetLocalRotation() const { return mLocalRotation; }
		inline const auto& GetLocalScale() const { return mLocalScale; }
		inline const auto& GetLocalMatrix() const { return mLocalMatrix; }

		inline const auto& GetWorldPosition() const { return mWorldPosition; }
		inline const auto& GetWorldRotation() const { return mWorldRotation; }
		inline const auto& GetWorldScale() const { return mWorldScale; }
		inline const auto& GetWorldMatrix() const { return mWorldMatrix; }

		inline const auto& IsStatic() const { return mIsStatic; }

	public:
		void SerializeJson(rapidjson::Document& doc) const override;

		void DeserializeJson(const rapidjson::Document& doc) override;

	private:
		// ��Gui�������ʱ��local���ݿɶ�д��world���ݽ��ɶ�

		Math::Vector3		mLocalPosition;		// ����ڸ��ڵ��λ��
		Math::Quaternion	mLocalRotation;		// ����ڸ��ڵ����ת
		Math::Vector3		mLocalScale;		// ����ڸ��ڵ������
		Math::Matrix4		mLocalMatrix;		// ����ڸ��ڵ�ı任
		
		Math::Vector3		mWorldPosition;		// ���������ԭ���λ��
		Math::Quaternion	mWorldRotation;		// ���������ԭ�����ת
		Math::Vector3		mWorldScale;		// ���������ԭ�������
		Math::Matrix4		mWorldMatrix;		// ���������ԭ��ı任

		bool				mIsStatic{ false };	// �Ƿ�Ϊ��̬����
	};
}