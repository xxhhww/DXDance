#pragma once
#include "Math/Vector.h"
#include <vector>

namespace Renderer {

	struct ClusterNode {
	public:
		Math::Vector4 minBoundingPosition;
		Math::Vector4 maxBoundingPosition;

		int32_t firstChild;	// 首孩子
		int32_t lastChild;	// 尾孩子

		int32_t firstInstance;	// 首instance
		int32_t lastInstance;	// 尾instance

		Math::Vector4 minInstanceScale;	// 最小缩放比例
		Math::Vector4 maxInstanceScale;	// 最大缩放比例
	};

	struct ClusterTree {
	public:
		std::vector<ClusterNode> clusterNodes;

	};

	struct ClusterBuilder {
	public:
		int32_t transformNums;


		struct FRunPair {	// 相当于区域 
			int32_t start;
			int32_t num;

			FRunPair(int32_t InStart, int32_t InNum)
				: start(InStart)
				, num(InNum) 
			{
			}

			bool operator< (const FRunPair& Other) const {
				return start < Other.start;
			}
		};
		std::vector<FRunPair> clusters;// Cluster是整个大群体

		struct FSortPair {	// 相当于point 
			float d;//最大轴向上面的值
			int32_t Index;

			bool operator< (const FSortPair& Other) const {
				return d < Other.d;
			}
		};
		std::vector<FSortPair> sortPairs;
	};

}