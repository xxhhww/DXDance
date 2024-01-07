#include "Renderer/ReTextureFileFormat.h"

#include "Tools/Assert.h"
#include "Tools/StrUtil.h"

#include <fstream>

namespace Renderer {

	ReTextureFileFormat::ReTextureFileFormat(const std::string& filepath) 
	: mFilepath(filepath) {
		std::ifstream inFile(filepath.c_str(), std::ios::binary);
		ASSERT_FORMAT(!inFile.fail(), "File Not Exists");

		inFile.read((char*)&mFileHeader, sizeof(FileHeader));
		ASSERT_FORMAT(inFile.good(), "Unexpected Error reading header");

		mSubresourceInfos.resize(mFileHeader.subresourceNums);
		inFile.read((char*)mSubresourceInfos.data(), mSubresourceInfos.size() * sizeof(SubresourceInfo));
		if (!inFile.good()) { ASSERT_FORMAT(false, "Unexpected Error reading subresourceInfos"); }

		mTileDataInfos.resize(mFileHeader.tileNums);
		inFile.read((char*)mTileDataInfos.data(), mTileDataInfos.size() * sizeof(TileDataInfo));
		if (!inFile.good()) { ASSERT_FORMAT(false, "Unexpected Error reading tileDataInfos"); }
	}

	ReTextureFileFormat::~ReTextureFileFormat() {
	}

	const std::string ReTextureFileFormat::GetFilename() const {
		return Tool::StrUtil::GetFilename(mFilepath);
	}

}