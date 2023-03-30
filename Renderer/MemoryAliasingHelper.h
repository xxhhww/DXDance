#pragma once
#include <set>
#include <vector>

namespace Renderer {

	class RenderGraphResource;
	class RenderGraphResourceStorage;

	/*
	* 帮助管线资源做显存布局
	*/
	class MemoryAliasingHelper {
	public:
		MemoryAliasingHelper(RenderGraphResourceStorage* storage);
		~MemoryAliasingHelper() = default;

		void AddResource(RenderGraphResource* resource);

		size_t BuildAliasing();

	private:
		struct MemoryRegion {
			uint64_t Offset;
			uint64_t Size;
		};

		enum class MemoryOffsetType {
			Start, End
		};

		using MemoryOffset = std::pair<uint64_t, MemoryOffsetType>;

	private:
		void AliasWithAlreadyAliasedAllocations(RenderGraphResource* resource);

		void BuildMemoryRegionForCurrentResource(RenderGraphResource* resource);

		void RemoveAliasedAllocations();

		bool TimelinesIntersect(const RenderGraphResource* a, const RenderGraphResource* b) const;

		void FitAliasableMemoryRegion(const MemoryRegion& nextAliasableRegion, uint64_t nextAllocationSize, MemoryRegion& optimalRegion) const;

		static bool Sort(RenderGraphResource* a, RenderGraphResource* b);

	private:
		RenderGraphResourceStorage* mResourceStorage{ nullptr };
		std::multiset<RenderGraphResource*, decltype(&MemoryAliasingHelper::Sort)> mNonAliasedResources;
		
		size_t mCurrBucketHeapOffset{ 0u };
		size_t mCurrBucketAvailableSize{ 0u };

		std::vector<MemoryOffset> mNonAliasableMemoryOffsets;
		std::vector<RenderGraphResource*> mAlreadyAliasedResources;
	};

}