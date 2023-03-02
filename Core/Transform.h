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
		* 设置相对于父节点的位置
		*/
		void SetLocalPosition(const Math::Vector3& position);

		/*
		* 设置相对于父节点的旋转
		*/
		void SetLocalRotation(const Math::Quaternion& rotation);

		/*
		* 设置相对于父节点的缩放
		*/
		void SetLocalScale(const Math::Vector3& scale);

		/*
		* 设置静态物体
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
		// 在Gui界面操作时，local数据可读写，world数据仅可读

		Math::Vector3		mLocalPosition;		// 相对于父节点的位置
		Math::Quaternion	mLocalRotation;		// 相对于父节点的旋转
		Math::Vector3		mLocalScale;		// 相对于父节点的缩放
		Math::Matrix4		mLocalMatrix;		// 相对于父节点的变换
		
		Math::Vector3		mWorldPosition;		// 相对于世界原点的位置
		Math::Quaternion	mWorldRotation;		// 相对于世界原点的旋转
		Math::Vector3		mWorldScale;		// 相对于世界原点的缩放
		Math::Matrix4		mWorldMatrix;		// 相对于世界原点的变换

		bool				mIsStatic{ false };	// 是否为静态物体
	};
}