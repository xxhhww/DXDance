#pragma once
#include "Math/Matrix.h"
#include <vector>

namespace Renderer {

	struct InstancedStaticMeshData {
	public:
		Math::Matrix4 worldTransform;	// 变换矩阵，将实例转换到世界空间
	};

	class HierarchyInstancedStaticMesh {
	public:
		HierarchyInstancedStaticMesh() = default;
		~HierarchyInstancedStaticMesh() = default;

		/*
		* 压入静态网格的实例化数据
		*/
		void Emplace(const Math::Matrix4& worldTransform);

		/*
		* 构建
		*/
		void Build();

	private:
		std::vector<InstancedStaticMeshData> mInstancedStaticMeshDatas;	// 实例化数据(全部上传至GPU)
	};

}