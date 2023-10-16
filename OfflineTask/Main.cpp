#include "OfflineTask/ResolveHDAFile.h"
#include "OfflineTask/SplitHeightMap.h"
#include "OfflineTask/GeneralTasks.h"
#include "HoudiniApi/HoudiniApi.h"
#include <Windows.h>

int main() {

	// HAPI��DLL��·����
	const std::wstring HAPIDllPath = L"E:/Houdini/bin/libHAPIL.dll";

	// ����HAPI��DLL��
	HINSTANCE HAPILibraryHandle = LoadLibrary((HAPIDllPath).c_str());

	// ��������HAPI�ӿں���
	FHoudiniApi::InitializeHAPI(HAPILibraryHandle);

	// ִ����������
	{
		OfflineTask::ResolveHDAFile resolveHDAFile;
		OfflineTask::SplitHeightMap splitHeightMap;

		OfflineTask::GeneralTasks::MergeTextures(
			"E:/MyProject/DXDance/Resources/Textures/OfflineTask/New/rChannelFile.png",
			"E:/MyProject/DXDance/Resources/Textures/OfflineTask/New/ClayMask.png",
			"E:/MyProject/DXDance/Resources/Textures/OfflineTask/New/Rock.png",
			"E:/MyProject/DXDance/Resources/Textures/OfflineTask/New/gChannelFile.png",
			"E:/MyProject/DXDance/Resources/Textures/OfflineTask/New/SplatMap.png"
		);

		// resolveHDAFile.Run("E:/MyProject/DXDance/Resources/HDAs/HE_Platform_maker.hda");
		// splitHeightMap.Run("E:/MyProject/DXDance/Resources/Textures/Terrain/HeightMap.png");
	}

	// �ͷ�DLL
	FreeLibrary(HAPILibraryHandle);

	return 0;

}