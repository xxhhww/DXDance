#pragma once
#include "pbh.h"

namespace GHL {

	struct BufferDesc {
		uint32_t          stride        = 1u;	                   // �����е�Ԫ�صĲ���
		size_t            size          = 0u;	                   // ��������ֽڸ���
		DXGI_FORMAT       format        = DXGI_FORMAT_UNKNOWN;     // ����Ԫ�صĸ�ʽ
		EResourceUsage    usage         = EResourceUsage::Upload;  // ��Դ����
		EResourceBindFlag bindFlag      = EResourceBindFlag::None; // ������Ҫ�󶨵���ͼ����(������)
		EBufferMiscFlag   miscFlag      = EBufferMiscFlag::None;   // �����������
		EResourceState    initalState   = EResourceState::Common;  // ���г�ʼ��״̬
		EResourceState    expectedState = EResourceState::Common;  // ����������״̬
		bool              placed        = false;                   // �Ƿ�ʹ��placed�ķ�ʽ��������
	};

	/*
	* һ���ò���
	*/
	struct BufferSubResourceDesc {
		uint64_t offset = 0u;
		uint64_t size   = uint64_t(-1);
	};

	struct TextureDesc {
		ETextureDimension dimension = ETextureDimension::Texture2D; // �����ά��
		uint32_t          width         = 0u;
		uint32_t          height        = 0u;
		uint32_t          depth         = 0u;
		uint32_t          arraySize     = 1u;
		uint32_t          mipLevals     = 1u;
		uint32_t          sampleCount   = 1u;
		DXGI_FORMAT       format        = DXGI_FORMAT_UNKNOWN;
		EResourceUsage    usage         = EResourceUsage::Default;
		ETextureMiscFlag  miscFlag      = ETextureMiscFlag::None;
		EResourceBindFlag bindFlag      = EResourceBindFlag::None;  // (������)
		EResourceState    initialState  = EResourceState::Common;
		EResourceState    expectedState = EResourceState::Common;
		bool              placed        = false;                    // �Ƿ�ʹ��placed�ķ�ʽ��������
		bool              reserved      = false;                    // �Ƿ�ʹ��reserved�ķ�ʽ��������
	};

	/*
	* ��������Դ����
	*/
	struct TextureSubResourceDesc {
		uint32_t firstSlice = 0u;
		uint32_t sliceCount = -1; // -1 ��ʾȫ��
		uint32_t firstMip   = 0u;
		uint32_t mipCount   = -1; // -1 ��ʾȫ��
	};

}