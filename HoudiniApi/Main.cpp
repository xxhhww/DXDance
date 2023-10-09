#include <string>
#include <Windows.h>
#include "HAPI/HAPI.h"
#include "HoudiniApi/HoudiniApi.h"

int main() {
	//HAPI的DLL的路径：
	const std::wstring HAPIDllPath = L"E:/Houdini/bin/libHAPIL.dll";

	//加载HAPI的DLL：
	HINSTANCE HAPILibraryHandle = LoadLibrary((HAPIDllPath).c_str());

	//加载所有HAPI接口函数
	FHoudiniApi::InitializeHAPI(HAPILibraryHandle);

	//一段测试
	{
		//创建session
		HAPI_Session session;
		FHoudiniApi::CreateInProcessSession(&session);

		//初始化session
		HAPI_CookOptions options;
		FHoudiniApi::Initialize(&session, &options, false, -1, "", "", "", "", "");

		//得到obj节点
		HAPI_NodeId ObjNode = -1;
		FHoudiniApi::GetManagerNodeId(&session, HAPI_NodeType::HAPI_NODETYPE_OBJ, &ObjNode);

		//创建Geo节点
		HAPI_NodeId GeoNode = -1;
		FHoudiniApi::CreateNode(&session, ObjNode, "geo", "MyGeo", true, &GeoNode);

		//创建一个测试用节点
		HAPI_NodeId TestNode = -1;
		FHoudiniApi::CreateNode(&session, GeoNode, "platonic", "MyTest", true, &TestNode);

		//设置其为二十面体
		FHoudiniApi::SetParmIntValue(&session, TestNode, "type", 0, 3);

		//测试输出到bgeo中
		FHoudiniApi::SaveGeoToFile(&session, TestNode, "E:/Houdini/TestGeo.bgeo");

		//清理并关闭Session
		FHoudiniApi::Cleanup(&session);
		FHoudiniApi::CloseSession(&session);
	}

	//释放DLL
	FreeLibrary(HAPILibraryHandle);

	return 0;
}