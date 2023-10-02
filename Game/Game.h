#pragma once
#include "Game/Context.h"
#include "Game/GameInitializer.h"

namespace Game {

	class Game {
	public:
		Game(Context& context);
		~Game();

		void Run();

		void Update(float delta);

	private:
		// 预处理函数
		void PreUpdate(float delta);

		// 编辑器后处理函数
		void PostUpdate(float delta);

	private:
		bool mPause{ true };
		Context& mContext;
		GameInitializer mGameInitializer;
	};

}