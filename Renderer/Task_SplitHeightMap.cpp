#include "Renderer/Task_SplitHeightMap.h"
#include "DirectXTex/DirectXTex.h"
#include "Tools/Assert.h"

#include <wincodec.h>
#include <fstream>

namespace Renderer {

	void Task_SplitHeightMap::Run(const std::string& filename) {

        static bool coInitialized = false;
        if (!coInitialized) {
            CoInitialize(nullptr);
        }

        /*
        * 读取纹理数据至内存
        */
        DirectX::ScratchImage srcImage;
        HRASSERT(DirectX::LoadFromWICFile(
            Tool::StrUtil::UTF8ToWString(filename).c_str(),
            DirectX::WIC_FLAGS::WIC_FLAGS_NONE,
            nullptr,
            srcImage
        ));

        // 计算均分后每一张图像的基础数据
        DirectX::Image dstImage;
        dstImage.width = srcImage.GetImages()[0].width / smTileCountPerAxis;
        dstImage.height = srcImage.GetImages()[0].height / smTileCountPerAxis;
        dstImage.format = srcImage.GetImages()[0].format;
        dstImage.rowPitch = srcImage.GetImages()[0].rowPitch / smTileCountPerAxis;
        dstImage.slicePitch = srcImage.GetImages()[0].slicePitch / (smTileCountPerAxis * smTileCountPerAxis);
        dstImage.pixels = new uint8_t[dstImage.slicePitch];

        for (uint32_t row = 0; row < smTileCountPerAxis; row++) {
            for (uint32_t col = 0; col < smTileCountPerAxis; col++) {

                // 计算均分后图像相对于原始图形的起始偏移量
                uint64_t srcOffset = (smTileCountPerAxis * row) * dstImage.slicePitch + col * dstImage.rowPitch;
                uint64_t dstOffset = 0u;

                for (uint32_t index = 0; index < dstImage.height; index++) {
                    memcpy(dstImage.pixels + dstOffset, srcImage.GetImages()[0].pixels + srcOffset, dstImage.rowPitch);

                    srcOffset += srcImage.GetImages()[0].rowPitch;
                    dstOffset += dstImage.rowPitch;
                }


                uint32_t binOffset = 0u;
                std::vector<float> heightDatas;
                for (uint32_t index = 0; index < dstImage.height; index++) {
                    for (uint32_t groupIndex = 0; groupIndex < dstImage.width; groupIndex++) {
                        // 读取一个组下的RGBA值
                        uint16_t u16R = 0;
                        uint16_t u16G = 0;
                        uint16_t u16B = 0;
                        uint16_t u16A = 0;
                        memcpy(&u16R, dstImage.pixels + binOffset, sizeof(uint16_t)); binOffset += 2u;
                        memcpy(&u16G, dstImage.pixels + binOffset, sizeof(uint16_t)); binOffset += 2u;
                        memcpy(&u16B, dstImage.pixels + binOffset, sizeof(uint16_t)); binOffset += 2u;
                        memcpy(&u16A, dstImage.pixels + binOffset, sizeof(uint16_t)); binOffset += 2u;

                        assert((u16R == u16G) && (u16G == u16B) && u16A == 65535);
                        float heightData = (float)u16R / (float)u16A;
                        heightDatas.push_back(heightData);
                    }
                }

                std::string binFilename = "HeightMap_" + std::to_string(row) + "_" + std::to_string(col) + ".bin";
                std::ofstream fout(binFilename.c_str(), std::ios::binary | std::ios::out);
                fout.write((char*)heightDatas.data(), heightDatas.size() * sizeof(float));
                fout.close();

                // 数据复制完成，存入磁盘(V轴命名需翻转)
                std::string filename = "HeightMap_" + std::to_string(row) + "_" + std::to_string(col) + ".png";
                DirectX::SaveToWICFile(dstImage, DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG), Tool::StrUtil::UTF8ToWString(filename).c_str());


            }

        }
	}

}