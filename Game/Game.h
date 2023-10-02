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
		// Ԥ������
		void PreUpdate(float delta);

		// �༭��������
		void PostUpdate(float delta);

	private:
		bool mPause{ true };
		Context& mContext;
		GameInitializer mGameInitializer;
	};

}