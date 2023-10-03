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

		// 处理玩家的额外操作
		void HandlePlayerInput();

	private:
		bool mPause{ true };
		Context& mContext;
		GameInitializer mGameInitializer;
	};

}