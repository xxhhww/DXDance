#include "Renderer/ClusterTree.h"
#include "Math/BoundingBox.h"
#include "Tools/Assert.h"
#include <algorithm>

namespace Renderer {

	ClusterBuilder::ClusterBuilder(
		const std::vector<Math::Matrix4>& _transform,
		const Math::BoundingBox& _instanceBoundingBox,
		int32_t _branchingFactor,
		int32_t _internalNodeBranchingFactor,
		int32_t _instancingRandomSeed,
		float _densityScaling,
		bool  _generateInstanceScalingRange) 
		: transforms(_transform)
		, instanceBoundingBox(_instanceBoundingBox)
		, branchingFactor(_branchingFactor)
		, internalNodeBranchingFactor(_internalNodeBranchingFactor)
		, instancingRandomSeed(_instancingRandomSeed)
		, densityScaling(_densityScaling)
		, generateInstanceScalingRange(_generateInstanceScalingRange) {

		int32_t transformNums = transforms.size();
		sortIndex.resize(transformNums);
		sortPoints.resize(transformNums);

		for (int32_t index = 0; index < transformNums; index++) {
			const auto& transform = transforms[index].Transpose();
			DirectX::XMVECTOR positionDx;
			DirectX::XMMatrixDecompose(nullptr, nullptr, &positionDx, transform);
			sortPoints[index] = positionDx;
			sortIndex[index] = index;
		}

		result = std::make_unique<ClusterTree>();
	}

	void ClusterBuilder::BuildTree(bool leafOnly) {

        int32_t transformNums = transforms.size();
		// 计算叶子节点
		Split(transformNums);

		int32_t rootNums = clusters.size();
		result->clusterNodes.resize(rootNums);
        std::vector<int32_t>& sortedInstances = result->sortedInstances;
        sortedInstances.insert(sortedInstances.end(), sortIndex.begin(), sortIndex.end());

		for (int32_t index = 0; index < rootNums; index ++) {
			ClusterNode& clusterNode = result->clusterNodes[index];
			clusterNode.firstInstance = clusters[index].start;
			clusterNode.lastInstance = clusters[index].start + clusters[index].num - 1;
			
			Math::BoundingBox clusterBoundingBox;
			for (int32_t instanceIndex = clusterNode.firstInstance; instanceIndex <= clusterNode.lastInstance; instanceIndex ++) {
				const Math::Matrix4& currInstanceTransform = transforms[sortedInstances[instanceIndex]].Transpose();
				Math::BoundingBox currInstanceBox = instanceBoundingBox.transformBy(currInstanceTransform);
                clusterBoundingBox += currInstanceBox;

				if (generateInstanceScalingRange) {
                    DirectX::XMVECTOR scalingDx;
                    DirectX::XMMatrixDecompose(&scalingDx, nullptr, nullptr, currInstanceTransform);
                    Math::Vector3 currInstanceScaling = scalingDx;

                    clusterNode.minInstanceScale = Math::Min(clusterNode.minInstanceScale, Math::Vector4{ currInstanceScaling, 0.0f });
                    clusterNode.maxInstanceScale = Math::Max(clusterNode.maxInstanceScale, Math::Vector4{ currInstanceScaling, 0.0f });
				}
			}
			clusterNode.minBoundingBoxPosition = clusterBoundingBox.minPosition;
			clusterNode.maxBoundingBoxPosition = clusterBoundingBox.maxPosition;
		}

        if (leafOnly) {
            return;
        }

        std::vector<int32_t> NodesPerLevel;
        NodesPerLevel.emplace_back(rootNums);
        int32_t LOD = 0;

        std::vector<int32_t> InverseSortIndex;
        std::vector<int32_t> RemapSortIndex;
        std::vector<int32_t> InverseInstanceIndex;
        std::vector<int32_t> OldInstanceIndex;
        std::vector<int32_t> LevelStarts;
        std::vector<int32_t> InverseChildIndex;
        std::vector<ClusterNode> OldNodes;
        // 自底向上，逐层处理
        while (rootNums > 1) {
            // 将当前层的每个节点（boundingbox的中心）当成一个点（实例）
            sortIndex.clear();
            sortPoints.clear();
            sortIndex.resize(rootNums);
            sortPoints.resize(rootNums);
            for (int32_t index = 0; index < rootNums; index++) {
                sortIndex[index] = index;
                ClusterNode& clusterNode = result->clusterNodes[index];
                sortPoints[index] = (clusterNode.minBoundingBoxPosition + clusterNode.maxBoundingBoxPosition) * 0.5f;
            }

            // 对上面的点（实例），重复此前划分（聚类）逻辑，不过这里每个cluster的实例数目从MaxInstancesPerLeaf变成了InternalNodeBranchingFactor，这个决定了分支的数目，比如当前层有100个节点，而InternalNodeBranchingFactor = 20，那么我们就得到了5个父节点，也就是说，每个父节点有20个子节点，那么这棵树就是20叉树
            branchingFactor = internalNodeBranchingFactor;

            Split(rootNums);

            InverseSortIndex.clear();
            InverseSortIndex.resize(rootNums);
            for (int32_t index = 0; index < rootNums; index++) {
                InverseSortIndex[sortIndex[index]] = index;
            }

            // 在sortInstances上不连续的ClusterNode可能合成一个父节点，因此需要对sortInstances重映射
            {
                RemapSortIndex.clear();
                RemapSortIndex.resize(transformNums);
                int32_t outIndex = 0;
                for (int32_t index = 0; index < rootNums; index++) {
                    ClusterNode& clusterNode = result->clusterNodes[sortIndex[index]];
                    for (int32_t instanceIndex = clusterNode.firstInstance; instanceIndex <= clusterNode.lastInstance; instanceIndex++) {
                        RemapSortIndex[outIndex++] = instanceIndex;
                    }
                }
                InverseInstanceIndex.clear();
                InverseInstanceIndex.resize(transformNums);
                for (int32_t index = 0; index < transformNums; index++) {
                    InverseInstanceIndex[RemapSortIndex[index]] = index;
                }
                for (int32_t index = 0; index < result->clusterNodes.size(); index++) {
                    ClusterNode& clusterNode = result->clusterNodes[index];
                    clusterNode.firstInstance = InverseInstanceIndex[clusterNode.firstInstance];
                    clusterNode.lastInstance = InverseInstanceIndex[clusterNode.lastInstance];
                }
                OldInstanceIndex.clear();
                std::swap(OldInstanceIndex, sortedInstances);
                sortedInstances.resize(transformNums);
                for (int32_t index = 0; index < transformNums; index++) {
                    sortedInstances[index] = OldInstanceIndex[RemapSortIndex[index]];
                }
            }

            // 对clusterNodes进行重排，新生成的父节点需要放在子节点的前面
            {
                int32_t NewNum = result->clusterNodes.size() + clusters.size();
                RemapSortIndex.clear();
                RemapSortIndex.resize(NewNum);
                LevelStarts.clear();
                LevelStarts.emplace_back(clusters.size());
                for (int32_t index = 0; index < NodesPerLevel.size() - 1; index++) {
                    LevelStarts.emplace_back(LevelStarts[index] + NodesPerLevel[index]);
                }

                for (int32_t Index = 0; Index < rootNums; Index++) {
                    ClusterNode& clusterNode = result->clusterNodes[sortIndex[Index]];
                    RemapSortIndex[LevelStarts[0]++] = sortIndex[Index];

                    int32_t LeftIndex = clusterNode.firstChild;
                    int32_t RightIndex = clusterNode.lastChild;
                    int32_t LevelIndex = 1;
                    while (RightIndex >= 0) {
                        int32_t NextLeftIndex = INT_MAX;
                        int32_t NextRightIndex = -1;
                        for (int32_t ChildIndex = LeftIndex; ChildIndex <= RightIndex; ChildIndex++) {
                            RemapSortIndex[LevelStarts[LevelIndex]++] = ChildIndex;
                            int32_t LeftChild = result->clusterNodes[ChildIndex].firstChild;
                            int32_t RightChild = result->clusterNodes[ChildIndex].lastChild;
                            if (LeftChild >= 0 && LeftChild < NextLeftIndex) {
                                NextLeftIndex = LeftChild;
                            }
                            if (RightChild >= 0 && RightChild > NextRightIndex) {
                                NextRightIndex = RightChild;
                            }
                        }
                        LeftIndex = NextLeftIndex;
                        RightIndex = NextRightIndex;
                        LevelIndex++;
                    }
                }
                ASSERT_FORMAT(LevelStarts[LevelStarts.size() - 1] == NewNum);
                InverseChildIndex.clear();
                InverseChildIndex.resize(NewNum);
                for (int32_t index = clusters.size(); index < NewNum; index++) {
                    InverseChildIndex[RemapSortIndex[index]] = index;
                }
                for (int32_t index = 0; index < result->clusterNodes.size(); index++) {
                    ClusterNode& clusterNode = result->clusterNodes[index];
                    if (clusterNode.firstChild >= 0) {
                        clusterNode.firstChild = InverseChildIndex[clusterNode.firstChild];
                        clusterNode.lastChild = InverseChildIndex[clusterNode.lastChild];
                    }
                }
                {
                    std::swap(OldNodes, result->clusterNodes);
                    result->clusterNodes.resize(NewNum);
                    for (int32_t index = 0; index < clusters.size(); index++) {
                        result->clusterNodes[index] = ClusterNode();
                    }
                    for (int32_t Index = 0; Index < OldNodes.size(); Index++) {
                        result->clusterNodes[InverseChildIndex[Index]] = OldNodes[Index];
                    }
                }
                int32_t OldIndex = clusters.size();
                int32_t instanceTracker = 0;
                for (int32_t index = 0; index < clusters.size(); index++) {
                    ClusterNode& clusterNode = result->clusterNodes[index];
                    clusterNode.firstChild = OldIndex;
                    OldIndex += clusters[index].num;
                    clusterNode.lastChild = OldIndex - 1;
                    clusterNode.firstInstance = result->clusterNodes[clusterNode.firstChild].firstInstance;
                    ASSERT_FORMAT(clusterNode.firstInstance == instanceTracker);
                    clusterNode.lastInstance = result->clusterNodes[clusterNode.lastChild].lastInstance;
                    instanceTracker = clusterNode.lastInstance + 1;
                    ASSERT_FORMAT(instanceTracker <= transformNums);

                    Math::BoundingBox nodeBoundingBox;
                    for (int32_t childIndex = clusterNode.firstChild; childIndex <= clusterNode.lastChild; childIndex++) {
                        ClusterNode& childNode = result->clusterNodes[childIndex];
                        nodeBoundingBox += childNode.minBoundingBoxPosition;
                        nodeBoundingBox += childNode.maxBoundingBoxPosition;
                        /*
                        if (generateInstanceScalingRange) {
                            Node.MinInstanceScale = Node.MinInstanceScale.ComponentMin(childNode.MinInstanceScale);
                            Node.MaxInstanceScale = Node.MaxInstanceScale.ComponentMax(childNode.MaxInstanceScale);
                        }
                        */
                    }
                    clusterNode.minBoundingBoxPosition = nodeBoundingBox.minPosition;
                    clusterNode.maxBoundingBoxPosition = nodeBoundingBox.maxPosition;
                }
                rootNums = clusters.size();
                NodesPerLevel.insert(NodesPerLevel.begin(), rootNums);
            }
        }

	}

	void ClusterBuilder::Split(int32_t nums) {
		clusters.clear();
		ClusterBuilder::Split(0, nums - 1);
		std::sort(clusters.begin(), clusters.end(), [](const RunPair& a, const RunPair& b) {
			return a.start < b.start;
		});
	}

	void ClusterBuilder::Split(int32_t start, int32_t end) {

		int32_t rangeNums = end - start + 1;

		// 计算包围盒大小
		Math::BoundingBox boundingBox;
		for (int32_t index = start; index <= end; index++) {
			boundingBox += sortPoints[sortIndex[index]];
		}

		// 不再继续划分 
		if (rangeNums <= branchingFactor){
			clusters.emplace_back(RunPair{ start, rangeNums });
			return;
		}

		sortPairs.clear();
		int32_t bestAxis = -1;
		float bestAxisValue = -1.0f;
		// 计算包围盒最长轴向
		for (int32_t axisIndex = 0; axisIndex < 3; axisIndex ++) {
			float thisAxisValue = 0.0f;
			if (axisIndex == 0) {
				thisAxisValue = boundingBox.maxPosition.x - boundingBox.minPosition.x;
			}
			else if (axisIndex == 1) {
				thisAxisValue = boundingBox.maxPosition.y - boundingBox.minPosition.y;
			}
			else if (axisIndex == 2) {
				thisAxisValue = boundingBox.maxPosition.z - boundingBox.minPosition.z;
			}

			if (!axisIndex || thisAxisValue > bestAxisValue) {
				bestAxis = axisIndex;			// 确定最长轴向 通过循环确定到底是 X Y Z
				bestAxisValue = thisAxisValue;	// 确定最长轴向的 最长轴向的值
			}
		}

		// 将sortpoints的index与pair对应起来 并加入到sortpairs中
		for (int32_t index = start; index <= end; index++) {
			SortPair sortPair;

			sortPair.index = sortIndex[index];
			if (bestAxis == 0) {
				sortPair.d = sortPoints[sortPair.index].x;
			}
			else if (bestAxis == 1) {
				sortPair.d = sortPoints[sortPair.index].y;
			}
			else if (bestAxis == 2) {
				sortPair.d = sortPoints[sortPair.index].z;
			}
			sortPairs.emplace_back(sortPair);
		}

		// 按照d值排序
		std::sort(sortPairs.begin(), sortPairs.end(), [](const SortPair& a, const SortPair& b) {
			return a.d < b.d;
		});

		// 将该区域内的sortIndex重新排序
		for (int32_t index = start; index <= end; index ++) {
			sortIndex[index] = sortPairs[index - start].index;
		}

		int32_t half = rangeNums / 2;	// 二分

		int32_t endLeft = start + half - 1;		// 左边区域的最后一个instance的ID
		int32_t startRight = 1 + end - half;	// 右边区域的第一个ID

		// 如果是奇数的情况 endleft和startright的值的变化
		if (rangeNums & 1) {
			if (sortPairs[half].d - sortPairs[half - 1].d < sortPairs[half + 1].d - sortPairs[half].d) {
				endLeft++;
			}
			else {
				startRight--;
			}
		}
		ASSERT_FORMAT(endLeft + 1 == startRight);
		ASSERT_FORMAT(endLeft >= start);
		ASSERT_FORMAT(end >= startRight);

		Split(start, endLeft);	// 分左块
		Split(startRight, end);	// 分右块
	}

}