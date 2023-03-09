#pragma once
#include <string>

namespace GHL {
	/*
	* DirectX12的对象的基类
	*/
	class D3DObject {
	public:
        /*
        * 设置D3DObject的调试名称
        */
        virtual void SetDebugName(const std::string& name) = 0;

        /*
        * 默认构造函数
        */
        D3DObject() = default;
        D3DObject(D3DObject&& that) = default;
        D3DObject& operator=(D3DObject&& that) = default;
        /*
        * 默认虚析构函数
        */
        virtual ~D3DObject() = default;
    protected:
        D3DObject(const D3DObject& that) = default;
        D3DObject& operator=(const D3DObject& that) = default;
	};
}