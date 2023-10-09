#include "OfflineTask/ResolveHDAFile.h"
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
		resolveHDAFile.Run("E:/MyProject/DXDance/Resources/HDAs/HE_Platform_maker.hda");
	}

	// �ͷ�DLL
	FreeLibrary(HAPILibraryHandle);

	return 0;

}