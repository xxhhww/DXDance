#pragma once
#include <string>

namespace OfflineTask {

	/*
	* 对高度图进行倒置
	*/
	class GeneralTasks {
	public:

		static void InvertTexture(const std::string& filename);

		static void SubstractTextures(
			const std::string& aMaskFile,
			const std::string& bMaskFile,
			const std::string& outputFile
		);

		static void SubstractTextures(
			const std::string& allMaskFile,
			const std::string& aMaskFile,
			const std::string& bMaskFile,
			const std::string& cMaskFile,
			const std::string& outputFile
		);

		static void OverRangeTextures(
			const std::string& rChannelFile,
			const std::string& gChannelFile);

		static void MergeTextures(
			const std::string& rChannelFile,
			const std::string& gChannelFile,
			const std::string& bChannelFile,
			const std::string& aChannelFile,
			const std::string& outputFile);

	};

}