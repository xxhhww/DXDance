#pragma once
#include <string>

namespace Renderer {

	/*
	* �ָ�߶���ͼ����
	*/
	class Task_SplitHeightMap {
	public:
		void Run(const std::string& filename);

	private:
		inline static uint32_t smTileCountPerAxis = 8u;
	};

}