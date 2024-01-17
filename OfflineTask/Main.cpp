#include "OfflineTask/ResolveHDAFile.h"
#include "OfflineTask/GeneralTasks.h"
#include "OfflineTask/GenerateGrassBlade.h"
#include "OfflineTask/TextureProcessor.h"
#include "HoudiniApi/HoudiniApi.h"
#include "GHL/DebugLayer.h"
#include "Windows/Window.h"
#include "Renderer/RenderEngine.h"
#include "Tools/Clock.h"

#include <Windows.h>

using namespace Renderer;
using namespace Windows;
using namespace OfflineTask;

inline static uint32_t sWindowWidth = 1920u;
inline static uint32_t sWindowHeight = 1080u;

void DoGenerateGrassBladeTask() {
    GHL::EnableDebugLayer();

    WindowSetting setting{};
    setting.width = sWindowWidth;
    setting.height = sWindowHeight;
    setting.fullscreen = false;
    setting.maximized = false;
    Window window{ setting };

    RenderEngine renderEngine(window.GetHWND(), setting.width, setting.height);
    GenerateGrassBlade generateGrassBlade;
    generateGrassBlade.Initialize(
        "E:/MyProject/DXDance/Resources/Textures/Terrain/Mountain01/HeightMap.png",
        "E:/MyProject/DXDance/Resources/Textures/Terrain/Mountain01/GrassLayer.png",
        "E:/MyProject/DXDance/Resources/Textures/Grass/GrassBlockData",
        &renderEngine
    );
    renderEngine.mOfflineTaskPass += std::bind(
        &GenerateGrassBlade::Generate, &generateGrassBlade,
        std::placeholders::_1, std::placeholders::_2);

    renderEngine.mOfflineCompletedCallback += std::bind(
        &GenerateGrassBlade::OnCompleted, &generateGrassBlade
    );

    Tool::Clock clock;
    bool done = false;
    while (!done) {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                done = true;
            }
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
        if (done) {
            break;
        }

        renderEngine.DoOfflineTask();
        clock.Update();
    }
}

void GenerateHeightMap() {
    /*
    TextureProcessor::Padding(
        "E:/MyProject/DXDance/Resources/Textures/Terrain/Mountain01/HeightMap.png",
        "E:/MyProject/DXDance/Resources/Textures/Terrain/Mountain01/Padding/", 1);
    */

    uint32_t maxLOD = 4;
    float	 minLODNodeMeterSize{ 64.0f };		// LOD0对应的地块大小为64.0f
    float    worldMeterSize{ 8192.0f };
    uint32_t nodeStartOffset{ 0u };
    for (int32_t i = maxLOD; i >= 0; i--) {
        float currLODNodeMeterSize = pow(2, i) * minLODNodeMeterSize;
        /*
        TextureProcessor::Split(
            "E:/MyProject/DXDance/Resources/Textures/Terrain/Mountain01/Padding/HeightMap.png",
            "E:/MyProject/DXDance/Resources/Textures/Terrain/Mountain01/Split/",
            currLODNodeMeterSize + 1, currLODNodeMeterSize, nodeStartOffset);
        */
        uint32_t nodeCountPerAxis = worldMeterSize / currLODNodeMeterSize;
        nodeStartOffset += nodeCountPerAxis * nodeCountPerAxis;
    }

    for (uint32_t i = 0; i < nodeStartOffset; i++) {
        /*
        TextureProcessor::Resize(
            "E:/MyProject/DXDance/Resources/Textures/Terrain/Mountain01/Split/HeightMap" + std::to_string(i) + ".png",
            "E:/MyProject/DXDance/Resources/Textures/Terrain/Mountain01/Resize/",
            65, 65);
        */
    }
}

void GenerateFarTerrainAtlas(bool needPrepare) {

    {
        if (needPrepare) {
            // TextureProcessor::Padding("E:/TerrainOfflineTask/001/TerrainFar/HeightMap/HeightMap.png", "E:/TerrainOfflineTask/001/TerrainFar/HeightMap/Padding/", 1);
        
            uint32_t maxLOD = 4;
            float	 minLODNodeMeterSize{ 64.0f };		// LOD0对应的地块大小为64.0f
            float    worldMeterSize{ 8192.0f };
            uint32_t nodeStartOffset{ 0u };
            for (int32_t i = maxLOD; i >= 0; i--) {
                float currLODNodeMeterSize = pow(2, i) * minLODNodeMeterSize;
                // TextureProcessor::Split("E:/TerrainOfflineTask/001/TerrainFar/HeightMap/Padding/HeightMap.png", "E:/TerrainOfflineTask/001/TerrainFar/HeightMap/Split/", currLODNodeMeterSize + 1, currLODNodeMeterSize, nodeStartOffset);
                uint32_t nodeCountPerAxis = worldMeterSize / currLODNodeMeterSize;
                nodeStartOffset += nodeCountPerAxis * nodeCountPerAxis;
            }

            for (uint32_t i = 0; i < nodeStartOffset; i++) {
                // TextureProcessor::Resize("E:/TerrainOfflineTask/001/TerrainFar/HeightMap/Split/HeightMap" + std::to_string(i) + ".png", "E:/TerrainOfflineTask/001/TerrainFar/HeightMap/Resize/", 65, 65);
            }

            TextureProcessor::GenerateTextureAtlasFile1("E:/TerrainOfflineTask/001/TerrainFar/HeightMap/Resize/", "HeightMap", 0, nodeStartOffset - 1, "E:/TerrainOfflineTask/001/TerrainFar/HeightMap/TerrainHeightMapAtlas.ret");
        }
        /*
        if (needPrepare) {
            TextureProcessor::Padding("E:/TerrainOfflineTask/001/TerrainFar/AlbedoMap/AlbedoMap.png", "E:/TerrainOfflineTask/001/TerrainFar/AlbedoMap/Padding/", 1);

            uint32_t maxLOD = 4;
            float	 minLODNodeMeterSize{ 64.0f };		// LOD0对应的地块大小为64.0f
            float    worldMeterSize{ 8192.0f };
            uint32_t nodeStartOffset{ 0u };
            for (int32_t i = maxLOD; i >= 0; i--) {
                float currLODNodeMeterSize = pow(2, i) * minLODNodeMeterSize;
                TextureProcessor::Split("E:/TerrainOfflineTask/001/TerrainFar/AlbedoMap/Padding/AlbedoMap.png", "E:/TerrainOfflineTask/001/TerrainFar/AlbedoMap/Split/", currLODNodeMeterSize + 1, currLODNodeMeterSize, nodeStartOffset);
                uint32_t nodeCountPerAxis = worldMeterSize / currLODNodeMeterSize;
                nodeStartOffset += nodeCountPerAxis * nodeCountPerAxis;
            }

            for (uint32_t i = 0; i < nodeStartOffset; i++) {
                TextureProcessor::Resize(
                    "E:/TerrainOfflineTask/001/TerrainFar/AlbedoMap/Split/AlbedoMap" + std::to_string(i) + ".png",
                    "E:/TerrainOfflineTask/001/TerrainFar/AlbedoMap/Resize/", 65, 65);
            }
        }
        TextureProcessor::GenerateTextureAtlasFile1("E:/TerrainOfflineTask/001/TerrainFar/AlbedoMap/Resize/", "AlbedoMap", 0, 21823, "E:/TerrainOfflineTask/001/TerrainFar/AlbedoMap/TerrainAlbedoMapAtlas.ret");
        */

        /*
        if (needPrepare) {
            TextureProcessor::Padding("E:/TerrainOfflineTask/001/TerrainFar/NormalMap/NormalMap.png", "E:/TerrainOfflineTask/001/TerrainFar/NormalMap/Padding/", 1);

            uint32_t maxLOD = 4;
            float	 minLODNodeMeterSize{ 64.0f };		// LOD0对应的地块大小为64.0f
            float    worldMeterSize{ 8192.0f };
            uint32_t nodeStartOffset{ 0u };
            for (int32_t i = maxLOD; i >= 0; i--) {
                float currLODNodeMeterSize = pow(2, i) * minLODNodeMeterSize;
                TextureProcessor::Split("E:/TerrainOfflineTask/001/TerrainFar/NormalMap/Padding/NormalMap.png", "E:/TerrainOfflineTask/001/TerrainFar/NormalMap/Split/", currLODNodeMeterSize + 1, currLODNodeMeterSize, nodeStartOffset);
                uint32_t nodeCountPerAxis = worldMeterSize / currLODNodeMeterSize;
                nodeStartOffset += nodeCountPerAxis * nodeCountPerAxis;
            }

            for (uint32_t i = 0; i < nodeStartOffset; i++) {
                TextureProcessor::Resize(
                    "E:/TerrainOfflineTask/001/TerrainFar/NormalMap/Split/NormalMap" + std::to_string(i) + ".png",
                    "E:/TerrainOfflineTask/001/TerrainFar/NormalMap/Resize/", 65, 65);
            }
        }
        TextureProcessor::GenerateTextureAtlasFile1("E:/TerrainOfflineTask/001/TerrainFar/NormalMap/Resize/", "NormalMap", 0, 21823, "E:/TerrainOfflineTask/001/TerrainFar/NormalMap/TerrainNormalMapAtlas.ret");
        */
    }
}

void GenerateNearTerrainAtlas(bool needPrepare) {
    TextureProcessor::GenerateTextureAtlasFile2("E:/TerrainOfflineTask/001/TerrainNear/TextureArray/TerrainAlbedoArray.dds", "E:/TerrainOfflineTask/001/TerrainNear/TextureArray/TerrainAlbedo.ret");
    TextureProcessor::GenerateTextureAtlasFile2("E:/TerrainOfflineTask/001/TerrainNear/TextureArray/TerrainNormalArray.dds", "E:/TerrainOfflineTask/001/TerrainNear/TextureArray/TerrainNormal.ret");
    // TextureProcessor::GenerateTextureAtlasFile2("E:/TerrainOfflineTask/001/TerrainNear/HeightMap/HeightMap.png", "E:/TerrainOfflineTask/001/TerrainNear/HeightMap/HeightMap.ret");
}

void GenerateTerrainNodeData() {
    TextureProcessor::GenerateTerrainQuadNodeDescriptors("E:/TerrainOfflineTask/001/TerrainNear/HeightMap/HeightMap.png", "E:/TerrainOfflineTask/001/TerrainNear/");
}

void GenerateSplatMap() {
    /*
    TextureProcessor::ConvertSplatMap(
        "E:/MyProject/DXDance/Resources/Textures/Terrain/Mountain01/SplatMaps/", "SplatMap",
        0, 0,
        "E:/MyProject/DXDance/Resources/Textures/Terrain/Mountain01/SplatMap.dds");
        */

    /*
    TextureProcessor::ResolveConvertedSplatMap(
        "E:/MyProject/DXDance/Resources/Textures/Terrain/Mountain01/SplatMap.dds",
        "E:/MyProject/DXDance/Resources/Textures/Terrain/Mountain01/ResolvedSplatMap.png"
    );
    */

    TextureProcessor::Padding(
        "E:/MyProject/DXDance/Resources/Textures/Terrain/Mountain01/HeightMap.png",
        "E:/MyProject/DXDance/Resources/Textures/Terrain/Mountain01/Padding/", 1);
}

void GenerateRandomMap() {
    TextureProcessor::GenerateRandomAlbedoMap("E:/MyProject/DXDance/Resources/Textures/Terrain/Mountain01/RandomAlbedoMap.png", 8192, 8192);
}

void GenerateTerrainTextureArray() {
    TextureProcessor::GenerateTextureArray("E:/TerrainOfflineTask/001/Albedo", "E:/TerrainOfflineTask/001/TerrainAlbedoArray.dds");
    TextureProcessor::GenerateTextureArray("E:/TerrainOfflineTask/001/Normal", "E:/TerrainOfflineTask/001/TerrainNormalArray.dds");
}

void GenerateReservedTextureFile() {
    TextureProcessor::GenerateTextureAtlasFile2("E:/TerrainOfflineTask/001/TerrainAlbedoArray.dds", "E:/TerrainOfflineTask/001/TerrainAlbedo.ret");
    TextureProcessor::GenerateTextureAtlasFile2("E:/TerrainOfflineTask/001/TerrainNormalArray.dds", "E:/TerrainOfflineTask/001/TerrainNormal.ret");
}

int main() {

	// HAPI的DLL的路径：
	const std::wstring HAPIDllPath = L"E:/Houdini/bin/libHAPIL.dll";

	// 加载HAPI的DLL：
	HINSTANCE HAPILibraryHandle = LoadLibrary((HAPIDllPath).c_str());

	// 加载所有HAPI接口函数
	FHoudiniApi::InitializeHAPI(HAPILibraryHandle);

	// 执行离线任务
	{
        GenerateFarTerrainAtlas(true);
        // GenerateNearTerrainAtlas(true);
        // GenerateTerrainNodeData();
	}

	// 释放DLL
	FreeLibrary(HAPILibraryHandle);

	return 0;

}