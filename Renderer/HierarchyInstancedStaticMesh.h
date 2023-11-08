#pragma once
#include "Math/Matrix.h"
#include <vector>

namespace Renderer {

	struct InstancedStaticMeshData {
	public:
		Math::Matrix4 worldTransform;	// �任���󣬽�ʵ��ת��������ռ�
	};

	class HierarchyInstancedStaticMesh {
	public:
		HierarchyInstancedStaticMesh() = default;
		~HierarchyInstancedStaticMesh() = default;

		/*
		* ѹ�뾲̬�����ʵ��������
		*/
		void Emplace(const Math::Matrix4& worldTransform);

		/*
		* ����
		*/
		void Build();

	private:
		std::vector<InstancedStaticMeshData> mInstancedStaticMeshDatas;	// ʵ��������(ȫ���ϴ���GPU)
	};

}