#include <string>
#include <Windows.h>
#include "HAPI/HAPI.h"
#include "HoudiniApi/HoudiniApi.h"

int main() {
	//HAPI��DLL��·����
	const std::wstring HAPIDllPath = L"E:/Houdini/bin/libHAPIL.dll";

	//����HAPI��DLL��
	HINSTANCE HAPILibraryHandle = LoadLibrary((HAPIDllPath).c_str());

	//��������HAPI�ӿں���
	FHoudiniApi::InitializeHAPI(HAPILibraryHandle);

	//һ�β���
	{
		//����session
		HAPI_Session session;
		FHoudiniApi::CreateInProcessSession(&session);

		//��ʼ��session
		HAPI_CookOptions options;
		FHoudiniApi::Initialize(&session, &options, false, -1, "", "", "", "", "");

		//�õ�obj�ڵ�
		HAPI_NodeId ObjNode = -1;
		FHoudiniApi::GetManagerNodeId(&session, HAPI_NodeType::HAPI_NODETYPE_OBJ, &ObjNode);

		//����Geo�ڵ�
		HAPI_NodeId GeoNode = -1;
		FHoudiniApi::CreateNode(&session, ObjNode, "geo", "MyGeo", true, &GeoNode);

		//����һ�������ýڵ�
		HAPI_NodeId TestNode = -1;
		FHoudiniApi::CreateNode(&session, GeoNode, "platonic", "MyTest", true, &TestNode);

		//������Ϊ��ʮ����
		FHoudiniApi::SetParmIntValue(&session, TestNode, "type", 0, 3);

		//���������bgeo��
		FHoudiniApi::SaveGeoToFile(&session, TestNode, "E:/Houdini/TestGeo.bgeo");

		//�����ر�Session
		FHoudiniApi::Cleanup(&session);
		FHoudiniApi::CloseSession(&session);
	}

	//�ͷ�DLL
	FreeLibrary(HAPILibraryHandle);

	return 0;
}