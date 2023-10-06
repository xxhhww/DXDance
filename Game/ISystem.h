#pragma once

namespace Game {

	class ISystem {
	public:
		virtual void Create() = 0;

		virtual void Destory() = 0;

		virtual void PrePhysicsUpdate() = 0;

		virtual void PostPhysicsUpdate() = 0;

		inline const auto& IsEnable() const { return mIsEnable; }

	private:
		bool mIsEnable{ true };
	};

}