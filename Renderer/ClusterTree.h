#pragma once
#include "Math/Vector.h"
#include <vector>

namespace Renderer {

	struct ClusterNode {
	public:
		Math::Vector4 minBoundingPosition;
		Math::Vector4 maxBoundingPosition;

		int32_t firstChild;	// �׺���
		int32_t lastChild;	// β����

		int32_t firstInstance;	// ��instance
		int32_t lastInstance;	// βinstance

		Math::Vector4 minInstanceScale;	// ��С���ű���
		Math::Vector4 maxInstanceScale;	// ������ű���
	};

	struct ClusterTree {
	public:
		std::vector<ClusterNode> clusterNodes;

	};

	struct ClusterBuilder {
	public:
		int32_t transformNums;


		struct FRunPair {	// �൱������ 
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
		std::vector<FRunPair> clusters;// Cluster��������Ⱥ��

		struct FSortPair {	// �൱��point 
			float d;//������������ֵ
			int32_t Index;

			bool operator< (const FSortPair& Other) const {
				return d < Other.d;
			}
		};
		std::vector<FSortPair> sortPairs;
	};

}