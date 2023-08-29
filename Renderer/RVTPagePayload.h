#pragma once
#include "Math/Int.h"

namespace Renderer {

	/*
	* Cell����
	*/
	struct RVTPagePayload {
	public:
        inline static Math::Int2 smInvalidTileIndex = Math::Int2(-1, -1);

        // ��Ӧƽ����ͼ�е�id
        Math::Int2 tileIndex{ smInvalidTileIndex };

        // �����֡���
        // public int ActiveFrame;

        // ��Ⱦ����
        // public RenderTextureRequest LoadRequest;

    public:
        // �Ƿ��ڿ���״̬
        inline bool IsReady() {
            return (tileIndex != smInvalidTileIndex);
        }

        // ��������
        inline void Reset() {
            tileIndex = smInvalidTileIndex;
        }
	};

}