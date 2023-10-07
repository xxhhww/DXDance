#pragma once
#include <string>

namespace Renderer {

	/*
	* 分割高度贴图任务
	*/
	class Task_SplitHeightMap {
	public:
		void Run(const std::string& filename);

	private:
		inline static uint32_t smTileCountPerAxis = 8u;
	};

}