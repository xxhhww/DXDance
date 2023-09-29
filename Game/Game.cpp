#include "Game/Game.h"

namespace Game {

	Game::Game(Context& context) 
	: mContext(context) {
	}

	Game::~Game() {
	}

	void Game::Run() {
		float delta = mContext.clock->GetDeltaTime();

		PreUpdate(delta);
		Update(delta);
		PostUpdate(delta);

		mContext.clock->Update();
	}

	void Game::Update(float delta) {

	}

	void Game::PreUpdate(float delta) {
		mContext.inputManger->PreUpdate(delta);
	}

	void Game::PostUpdate(float delta) {
		mContext.inputManger->PostUpdate();
	}

}