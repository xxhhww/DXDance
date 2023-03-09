#pragma once
#include <string>

namespace GHL {
	/*
	* DirectX12�Ķ���Ļ���
	*/
	class D3DObject {
	public:
        /*
        * ����D3DObject�ĵ�������
        */
        virtual void SetDebugName(const std::string& name) = 0;

        /*
        * Ĭ�Ϲ��캯��
        */
        D3DObject() = default;
        D3DObject(D3DObject&& that) = default;
        D3DObject& operator=(D3DObject&& that) = default;
        /*
        * Ĭ������������
        */
        virtual ~D3DObject() = default;
    protected:
        D3DObject(const D3DObject& that) = default;
        D3DObject& operator=(const D3DObject& that) = default;
	};
}