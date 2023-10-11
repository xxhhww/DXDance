#pragma once
#include "HoudiniApi/HoudiniApi.h"
#include "HoudiniApi/HoudiniApiUtility.h"
#include "HoudiniApi/HoudiniAsset.h"
#include "HoudiniApi/HoudiniObjectNode.h"
#include "HoudiniApi/HoudiniGeoNode.h"
#include "HoudiniApi/HoudiniPartData.h"

namespace OfflineTask {

	/*
	* ����HoudiniEngine������HDA�ļ�����������зָ�
	*/
	class ResolveHDAFile {
	public:
		void Run(const std::string& filepath);

	private:
		std::unique_ptr<HAPI_Session> mSession;
		std::unique_ptr<Houdini::HoudiniAsset> mHoudiniAsset;
	};

}