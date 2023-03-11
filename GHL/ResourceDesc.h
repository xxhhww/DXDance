#pragma once
#include "pbh.h"

namespace GHL {

	struct BufferDesc {
		uint32_t          stride   = 1u;	                  // �����е�Ԫ�صĲ���
		size_t            size     = 0u;	                  // ��������ֽڸ���
		EResourceBindFlag bindFlag = EResourceBindFlag::None; // ������Ҫ�󶨵���ͼ����
		EBufferMiscFlag   miscFlag = EBufferMiscFlag::None;   // �����������
	};

}