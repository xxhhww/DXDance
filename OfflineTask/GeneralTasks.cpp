#include "OfflineTask/GeneralTasks.h"
#include "DirectXTex/DirectXTex.h"
#include "Tools/Assert.h"
#include "Tools/StrUtil.h"

#include <wincodec.h>
#include <fstream>

namespace OfflineTask {

    void GeneralTasks::InvertTexture(const std::string& filename) {
        static bool coInitialized = false;
        if (!coInitialized) {
            CoInitialize(nullptr);
        }


        // 读取纹理数据至内存
        DirectX::ScratchImage srcImage;
        HRASSERT(DirectX::LoadFromWICFile(
            Tool::StrUtil::UTF8ToWString(filename).c_str(),
            DirectX::WIC_FLAGS::WIC_FLAGS_NONE,
            nullptr,
            srcImage
        ));

        // 倒置高度贴图
        DirectX::ScratchImage invertedImage;
        HRASSERT(DirectX::FlipRotate(
            *srcImage.GetImage(0, 0, 0),
            DirectX::TEX_FR_FLIP_VERTICAL,
            invertedImage
        ));

        // 保存倒置后的高度贴图
        std::string invertedFilename = "HeightMap_Inverted.png";
        HRASSERT(DirectX::SaveToWICFile(
            *invertedImage.GetImage(0, 0, 0),
            DirectX::WIC_FLAGS_NONE,
            DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG),
            Tool::StrUtil::UTF8ToWString(invertedFilename).c_str()
        ));
	}

    void GeneralTasks::SubstractTextures(
        const std::string& aMaskFile,
        const std::string& bMaskFile,
        const std::string& outputFile
    ) {
        static bool coInitialized = false;
        if (!coInitialized) {
            CoInitialize(nullptr);
        }

        DirectX::ScratchImage aMaskImage;
        HRASSERT(DirectX::LoadFromWICFile(
            Tool::StrUtil::UTF8ToWString(aMaskFile).c_str(),
            DirectX::WIC_FLAGS::WIC_FLAGS_NONE,
            nullptr,
            aMaskImage
        ));

        DirectX::ScratchImage bMaskImage;
        HRASSERT(DirectX::LoadFromWICFile(
            Tool::StrUtil::UTF8ToWString(bMaskFile).c_str(),
            DirectX::WIC_FLAGS::WIC_FLAGS_NONE,
            nullptr,
            bMaskImage
        ));

        DirectX::Image outputImage;
        outputImage.width = aMaskImage.GetImages()[0].width;
        outputImage.height = aMaskImage.GetImages()[0].height;
        outputImage.format = aMaskImage.GetImages()[0].format;
        outputImage.rowPitch = aMaskImage.GetImages()[0].rowPitch;
        outputImage.slicePitch = aMaskImage.GetImages()[0].slicePitch;
        outputImage.pixels = new uint8_t[outputImage.slicePitch];

        uint32_t dstOffset = 0u;
        uint32_t srcOffset = 0u;
        uint32_t count = 0u;
        for (uint32_t row = 0; row < outputImage.height; row++) {
            for (uint32_t col = 0; col < outputImage.width; col++) {
                uint8_t aMask = 0u;
                uint8_t bMask = 0u;

                memcpy(&aMask, aMaskImage.GetImage(0, 0, 0)->pixels + srcOffset, sizeof(uint8_t));
                memcpy(&bMask, bMaskImage.GetImage(0, 0, 0)->pixels + srcOffset, sizeof(uint8_t));
                srcOffset += 1u;

                uint8_t result = aMask - bMask;

                int32_t test = (int32_t)aMask - (int32_t)bMask;

                if (aMask != 0u && aMask < bMask) {
                    int32_t i = 0;
                }

                if (test < 0) {
                    count++;
                }

                memcpy(outputImage.pixels + dstOffset, &result, sizeof(uint8_t));
                dstOffset += 1u;
            }
        }

        // 数据复制完成，存入磁盘
        HRASSERT(DirectX::SaveToWICFile(
            outputImage,
            DirectX::WIC_FLAGS_NONE,
            DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG),
            Tool::StrUtil::UTF8ToWString(outputFile).c_str()
        ));

        delete outputImage.pixels;
    }

    void GeneralTasks::SubstractTextures(
        const std::string& allMaskFile,
        const std::string& aMaskFile,
        const std::string& bMaskFile,
        const std::string& cMaskFile,
        const std::string& outputFile
    ) {
        static bool coInitialized = false;
        if (!coInitialized) {
            CoInitialize(nullptr);
        }

        // 读取纹理数据至内存
        DirectX::ScratchImage allMaskImage;
        HRASSERT(DirectX::LoadFromWICFile(
            Tool::StrUtil::UTF8ToWString(allMaskFile).c_str(),
            DirectX::WIC_FLAGS::WIC_FLAGS_NONE,
            nullptr,
            allMaskImage
        ));

        DirectX::ScratchImage aMaskImage;
        HRASSERT(DirectX::LoadFromWICFile(
            Tool::StrUtil::UTF8ToWString(aMaskFile).c_str(),
            DirectX::WIC_FLAGS::WIC_FLAGS_NONE,
            nullptr,
            aMaskImage
        ));

        DirectX::ScratchImage bMaskImage;
        HRASSERT(DirectX::LoadFromWICFile(
            Tool::StrUtil::UTF8ToWString(bMaskFile).c_str(),
            DirectX::WIC_FLAGS::WIC_FLAGS_NONE,
            nullptr,
            bMaskImage
        ));

        DirectX::ScratchImage cMaskImage;
        HRASSERT(DirectX::LoadFromWICFile(
            Tool::StrUtil::UTF8ToWString(cMaskFile).c_str(),
            DirectX::WIC_FLAGS::WIC_FLAGS_NONE,
            nullptr,
            cMaskImage
        ));

        DirectX::Image outputImage;
        outputImage.width = allMaskImage.GetImages()[0].width;
        outputImage.height = allMaskImage.GetImages()[0].height;
        outputImage.format = allMaskImage.GetImages()[0].format;
        outputImage.rowPitch = allMaskImage.GetImages()[0].rowPitch;
        outputImage.slicePitch = allMaskImage.GetImages()[0].slicePitch;
        outputImage.pixels = new uint8_t[outputImage.slicePitch];

        uint32_t dstOffset = 0u;
        uint32_t srcOffset = 0u;
        uint32_t count = 0u;
        for (uint32_t row = 0; row < outputImage.height; row++) {
            for (uint32_t col = 0; col < outputImage.width; col++) {
                uint8_t allMask = 0u;
                uint8_t aMask = 0u;
                uint8_t bMask = 0u;
                uint8_t cMask = 0u;

                memcpy(&allMask, allMaskImage.GetImage(0, 0, 0)->pixels + srcOffset, sizeof(uint8_t));
                memcpy(&aMask, aMaskImage.GetImage(0, 0, 0)->pixels + srcOffset, sizeof(uint8_t));
                memcpy(&bMask, bMaskImage.GetImage(0, 0, 0)->pixels + srcOffset, sizeof(uint8_t));
                memcpy(&cMask, cMaskImage.GetImage(0, 0, 0)->pixels + srcOffset, sizeof(uint8_t));
                srcOffset += 1u;

                uint8_t result = allMask - aMask - bMask - cMask;

                int32_t test = (int32_t)allMask - (int32_t)aMask - (int32_t)bMask - (int32_t)cMask;

                if (test < 0) {
                    count++;
                    result = 0u;
                }

                memcpy(outputImage.pixels + dstOffset, &result, sizeof(uint8_t));
                dstOffset += 1u;
            }
        }

        // 数据复制完成，存入磁盘
        HRASSERT(DirectX::SaveToWICFile(
            outputImage,
            DirectX::WIC_FLAGS_NONE,
            DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG),
            Tool::StrUtil::UTF8ToWString(outputFile).c_str()
        ));

        delete outputImage.pixels;
    }

    void GeneralTasks::OverRangeTextures(
        const std::string& rChannelFile,
        const std::string& gChannelFile) {
        static bool coInitialized = false;
        if (!coInitialized) {
            CoInitialize(nullptr);
        }


        // 读取纹理数据至内存
        DirectX::ScratchImage rChannelScrImage;
        HRASSERT(DirectX::LoadFromWICFile(
            Tool::StrUtil::UTF8ToWString(rChannelFile).c_str(),
            DirectX::WIC_FLAGS::WIC_FLAGS_NONE,
            nullptr,
            rChannelScrImage
        ));

        DirectX::ScratchImage gChannelScrImage;
        HRASSERT(DirectX::LoadFromWICFile(
            Tool::StrUtil::UTF8ToWString(gChannelFile).c_str(),
            DirectX::WIC_FLAGS::WIC_FLAGS_NONE,
            nullptr,
            gChannelScrImage
        ));

        uint32_t srcOffset = 0u;
        uint32_t count = 0u;
        for (uint32_t row = 0; row < rChannelScrImage.GetImages()[0].height; row++) {
            for (uint32_t col = 0; col < rChannelScrImage.GetImages()[0].width; col++) {
                uint8_t rChannel = 0u;
                uint8_t gChannel = 0u;

                memcpy(&rChannel, rChannelScrImage.GetImage(0, 0, 0)->pixels + srcOffset, sizeof(uint8_t));
                memcpy(&gChannel, gChannelScrImage.GetImage(0, 0, 0)->pixels + srcOffset, sizeof(uint8_t));
                int32_t add = (int32_t)rChannel + (int32_t)gChannel;
                if (add > 255) {
                    if (rChannel > gChannel) rChannel--;
                    else gChannel--;

                    count++;
                }

                memcpy(rChannelScrImage.GetImage(0, 0, 0)->pixels + srcOffset, &rChannel, sizeof(uint8_t));
                memcpy(gChannelScrImage.GetImage(0, 0, 0)->pixels + srcOffset, &gChannel, sizeof(uint8_t));
                srcOffset++;
            }
        }

        // 数据复制完成，存入磁盘(V轴命名需翻转)
        HRASSERT(DirectX::SaveToWICFile(
            rChannelScrImage.GetImages()[0],
            DirectX::WIC_FLAGS_NONE,
            DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG),
            Tool::StrUtil::UTF8ToWString("rChannelFile.png").c_str()
        ));

        HRASSERT(DirectX::SaveToWICFile(
            gChannelScrImage.GetImages()[0],
            DirectX::WIC_FLAGS_NONE,
            DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG),
            Tool::StrUtil::UTF8ToWString("gChannelFile.png").c_str()
        ));

        int32_t i = 0;
    }

    void GeneralTasks::MergeTextures(
        const std::string& rChannelFile,
        const std::string& gChannelFile,
        const std::string& bChannelFile,
        const std::string& aChannelFile,
        const std::string& outputFile) {
        static bool coInitialized = false;
        if (!coInitialized) {
            CoInitialize(nullptr);
        }


        // 读取纹理数据至内存
        DirectX::ScratchImage rChannelScrImage;
        HRASSERT(DirectX::LoadFromWICFile(
            Tool::StrUtil::UTF8ToWString(rChannelFile).c_str(),
            DirectX::WIC_FLAGS::WIC_FLAGS_NONE,
            nullptr,
            rChannelScrImage
        ));

        DirectX::ScratchImage gChannelScrImage;
        HRASSERT(DirectX::LoadFromWICFile(
            Tool::StrUtil::UTF8ToWString(gChannelFile).c_str(),
            DirectX::WIC_FLAGS::WIC_FLAGS_NONE,
            nullptr,
            gChannelScrImage
        ));

        DirectX::ScratchImage bChannelScrImage;
        HRASSERT(DirectX::LoadFromWICFile(
            Tool::StrUtil::UTF8ToWString(bChannelFile).c_str(),
            DirectX::WIC_FLAGS::WIC_FLAGS_NONE,
            nullptr,
            bChannelScrImage
        ));

        DirectX::ScratchImage aChannelScrImage;
        HRASSERT(DirectX::LoadFromWICFile(
            Tool::StrUtil::UTF8ToWString(aChannelFile).c_str(),
            DirectX::WIC_FLAGS::WIC_FLAGS_NONE,
            nullptr,
            aChannelScrImage
        ));

        DirectX::Image outputDstImage;
        outputDstImage.width = rChannelScrImage.GetImages()[0].width;
        outputDstImage.height = rChannelScrImage.GetImages()[0].height;
        outputDstImage.format = DXGI_FORMAT_R8G8B8A8_UNORM; // 其他都是R8_UNORM
        outputDstImage.rowPitch = rChannelScrImage.GetImages()[0].rowPitch * 4;
        outputDstImage.slicePitch = rChannelScrImage.GetImages()[0].slicePitch * 4;
        outputDstImage.pixels = new uint8_t[outputDstImage.slicePitch];

        uint32_t dstOffset = 0u;
        uint32_t srcOffset = 0u;
        uint32_t count = 0u;
        for (uint32_t row = 0; row < outputDstImage.height; row++) {
            for (uint32_t col = 0; col < outputDstImage.width; col++) {
                uint8_t rChannel = 0u;
                uint8_t gChannel = 0u;
                uint8_t bChannel = 0u;
                uint8_t aChannel = 0u;

                memcpy(&rChannel, rChannelScrImage.GetImage(0, 0, 0)->pixels + srcOffset, sizeof(uint8_t));
                memcpy(&gChannel, gChannelScrImage.GetImage(0, 0, 0)->pixels + srcOffset, sizeof(uint8_t));
                memcpy(&bChannel, bChannelScrImage.GetImage(0, 0, 0)->pixels + srcOffset, sizeof(uint8_t));
                memcpy(&aChannel, aChannelScrImage.GetImage(0, 0, 0)->pixels + srcOffset, sizeof(uint8_t));
                srcOffset += 1u;

                int32_t add = (int32_t)rChannel + (int32_t)gChannel + (int32_t)bChannel + (int32_t)aChannel;
                if (add != 255) {
                    int i = 0;
                }
                memcpy(outputDstImage.pixels + dstOffset, &rChannel, sizeof(uint8_t)); dstOffset += 1u;
                memcpy(outputDstImage.pixels + dstOffset, &gChannel, sizeof(uint8_t)); dstOffset += 1u;
                memcpy(outputDstImage.pixels + dstOffset, &bChannel, sizeof(uint8_t)); dstOffset += 1u;
                memcpy(outputDstImage.pixels + dstOffset, &aChannel, sizeof(uint8_t)); dstOffset += 1u;
            }
        }
        count++;

        // 数据复制完成，存入磁盘(V轴命名需翻转)
        HRASSERT(DirectX::SaveToWICFile(
            outputDstImage,
            DirectX::WIC_FLAGS_NONE,
            DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG),
            Tool::StrUtil::UTF8ToWString(outputFile).c_str()
        ));

        delete outputDstImage.pixels;
    }

    void GeneralTasks::GenerateMipmaps(
        const std::string& sourceFile,
        bool splitMip
    ) {
        static bool coInitialized = false;
        if (!coInitialized) {
            CoInitialize(nullptr);
        }

        DirectX::ScratchImage sourceImage;
        HRASSERT(DirectX::LoadFromWICFile(
            Tool::StrUtil::UTF8ToWString(sourceFile).c_str(),
            DirectX::WIC_FLAGS::WIC_FLAGS_NONE,
            nullptr,
            sourceImage
        ));

        DirectX::ScratchImage mipChain;
        HRASSERT(DirectX::GenerateMipMaps(
            sourceImage.GetImages()[0], DirectX::TEX_FILTER_DEFAULT, 0, mipChain, false
        ));

        if (!splitMip) {
        }
        else {
            // 将MipChain拆分保存
            uint32_t mipLevels = mipChain.GetMetadata().mipLevels;
            for (uint32_t i = 0; i < mipLevels; i++) {
                std::string name = "MipChain" + std::to_string(i) + ".png";
                HRASSERT(DirectX::SaveToWICFile(
                    mipChain.GetImages()[i], 
                    DirectX::WIC_FLAGS_NONE, 
                    DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG), 
                    Tool::StrUtil::UTF8ToWString(name).c_str()
                ));
            }
        }
    }

}