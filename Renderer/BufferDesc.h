#pragma once
#include "GHL/pbh.h"

namespace Renderer {

	/*
	* ��������
	*/
	struct BufferDesc {
		uint32_t               stride        = 1u;	                         // �����е�Ԫ�صĲ���
		size_t                 size          = 0u;	                         // ��������ֽڸ���
		DXGI_FORMAT            format        = DXGI_FORMAT_UNKNOWN;          // ����Ԫ�صĸ�ʽ
		GHL::EResourceUsage    usage         = GHL::EResourceUsage::Upload;  // ��Դ����
		GHL::EBufferMiscFlag   miscFlag      = GHL::EBufferMiscFlag::None;   // �����������
		GHL::EResourceState    initalState   = GHL::EResourceState::Common;  // ���г�ʼ��״̬
		GHL::EResourceState    expectedState = GHL::EResourceState::Common;  // ����������״̬
	};

	/*
	* ��������Դ����
	*/
	struct BufferSubResourceDesc {
		size_t offset = 0u;
		size_t size   = static_cast<size_t>(-1);
	};

}