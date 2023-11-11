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
		std::vector<int32_t>     sortedInstances;
		std::vector<int32_t>     instanceReorderTable;
	};

	struct ClusterBuilder {
	public:
		std::unique_ptr<ClusterTree> result;

	public:
		Math::BoundingBox instanceBoundingBox;	// ʵ��������ľֲ��ռ��Χ��

		int32_t branchingFactor;				// ��֧����   
		int32_t internalNodeBranchingFactor;	// �ڲ��ڵ��֧����

		int32_t instancingRandomSeed;			// �����
		float   densityScaling;					// �ܶ�����
		bool    generateInstanceScalingRange;

		std::vector<int32_t> sortIndex;
		std::vector<Math::Vector3> sortPoints;
		std::vector<Math::Matrix4> transforms;
		std::vector<float> customDataFloats;


		struct RunPair {	// �൱������ 
			int32_t start;
			int32_t num;

			RunPair(int32_t InStart, int32_t InNum)
				: start(InStart)
				, num(InNum) {}

			bool operator< (const RunPair& Other) const {
				return start < Other.start;
			}
		};
		std::vector<RunPair> clusters;// clustersΪ��ʱ��Ҷ�ڵ�

		struct SortPair {	// �൱��point 
			float d;		// ������������ֵ
			int32_t index;

			bool operator< (const SortPair& Other) const {
				return d < Other.d;
			}
		};
		std::vector<SortPair> sortPairs;

	public:
		ClusterBuilder(
			const std::vector<Math::Matrix4>& _transform, 
			const Math::BoundingBox& _boundingBox,
			int32_t _branchingFactor = 512,
			int32_t _internalNodeBranchingFactor = 16,
			int32_t _instancingRandomSeed = 0, 
			float _densityScaling = 1.0f,
			bool  _generateInstanceScalingRange = false);

		~ClusterBuilder() = default;

		void BuildTree();

	private:
		void Split(int32_t num);

		void Split(int32_t start, int32_t end);
	};

}