#pragma once
#include "Math/Vector.h"
#include "Math/Matrix.h"
#include "Math/BoundingBox.h"
#include <vector>
#include <memory>

namespace Renderer {

	struct ClusterNode {
	public:
		Math::Vector4 minBoundingBoxPosition;
		Math::Vector4 maxBoundingBoxPosition;

		int32_t firstChild;	// 首孩子
		int32_t lastChild;	// 尾孩子

		int32_t firstInstance;	// 首instance
		int32_t lastInstance;	// 尾instance

		Math::Vector4 minInstanceScale{ INT_MAX, INT_MAX, INT_MAX, 0.0f };	// 最小缩放比例
		Math::Vector4 maxInstanceScale{ INT_MIN, INT_MIN, INT_MIN, 0.0f };	// 最大缩放比例
	};

	struct ClusterTree {
	public:
		std::vector<ClusterNode> clusterNodes;
		std::vector<int32_t>     sortedInstances;
		// std::vector<int32_t>     instanceReorderTable;
	};

	struct ClusterBuilder {
	public:
		std::unique_ptr<ClusterTree> result;

	public:
		Math::BoundingBox instanceBoundingBox;	// 实例化物体的局部空间包围盒

		int32_t branchingFactor;				// 分支因子
		int32_t internalNodeBranchingFactor;	// 内部节点分支因子

		int32_t instancingRandomSeed;			// 随机数
		float   densityScaling;					// 密度缩放
		bool    generateInstanceScalingRange;

		std::vector<int32_t> sortIndex;
		std::vector<Math::Vector3> sortPoints;
		std::vector<Math::Matrix4> transforms;
		// std::vector<float> customDataFloats;

		struct RunPair {	// 相当于区域 
			int32_t start;
			int32_t num;

			RunPair(int32_t InStart, int32_t InNum)
				: start(InStart)
				, num(InNum) {}

			bool operator< (const RunPair& Other) const {
				return start < Other.start;
			}
		};
		std::vector<RunPair> clusters;// clusters为临时的叶节点

		struct SortPair {	// 相当于point 
			float d;		// 沿最长轴向上面的值
			int32_t index;

			bool operator< (const SortPair& Other) const {
				return d < Other.d;
			}
		};
		std::vector<SortPair> sortPairs;

	public:
		ClusterBuilder(
			const std::vector<Math::Matrix4>& _transform, // 注意矩阵此时已被转置为GPU端，使用时需转置回CPU端
			const Math::BoundingBox& _instanceBoundingBox,
			int32_t _branchingFactor = 512,
			int32_t _internalNodeBranchingFactor = 16,
			int32_t _instancingRandomSeed = 0, 
			float _densityScaling = 1.0f,
			bool  _generateInstanceScalingRange = true);

		~ClusterBuilder() = default;

		/*
		* leafOnly: 只构建叶子节点
		*/
		void BuildTree(bool leafOnly = true);

	private:
		void Split(int32_t num);

		void Split(int32_t start, int32_t end);
	};

}