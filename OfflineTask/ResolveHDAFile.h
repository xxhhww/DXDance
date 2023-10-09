#pragma once
#include "HoudiniApi/HoudiniApi.h"
#include "HoudiniApi/HoudiniApiUtility.h"
#include <string>
#include <memory>
#include <vector>

namespace OfflineTask {

	/*
	* 链接HoudiniEngine来解算HDA文件，并对其进行分割
	*/
	class ResolveHDAFile {
	public:
		void Run(const std::string& filepath);

	private:
		std::unique_ptr<HAPI_Session> mSession;
	};

}