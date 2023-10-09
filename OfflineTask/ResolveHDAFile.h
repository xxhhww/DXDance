#pragma once
#include "HoudiniApi/HoudiniApi.h"
#include "HoudiniApi/HoudiniApiUtility.h"
#include <string>
#include <memory>
#include <vector>

namespace OfflineTask {

	/*
	* ����HoudiniEngine������HDA�ļ�����������зָ�
	*/
	class ResolveHDAFile {
	public:
		void Run(const std::string& filepath);

	private:
		std::unique_ptr<HAPI_Session> mSession;
	};

}