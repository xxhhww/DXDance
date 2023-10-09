#include "OfflineTask/ResolveHDAFile.h"
#include "HoudiniApi/HoudiniApi.h"
#include <Windows.h>

int main() {

	// HAPI的DLL的路径：
	const std::wstring HAPIDllPath = L"E:/Houdini/bin/libHAPIL.dll";

	// 加载HAPI的DLL：
	HINSTANCE HAPILibraryHandle = LoadLibrary((HAPIDllPath).c_str());

	// 加载所有HAPI接口函数
	FHoudiniApi::InitializeHAPI(HAPILibraryHandle);

	// 执行离线任务
	{
		OfflineTask::ResolveHDAFile resolveHDAFile;
		resolveHDAFile.Run("E:/MyProject/DXDance/Resources/HDAs/HE_Platform_maker.hda");
	}

	// 释放DLL
	FreeLibrary(HAPILibraryHandle);

	return 0;

}