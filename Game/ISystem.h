#pragma once

namespace Game {

	class ISystem {
	public:
		virtual void Create() {};

		virtual void Destory() {};

		virtual void Run() = 0;
	};

}