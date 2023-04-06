#pragma once
#include "ResourceFormat.h"
#include "RenderGraphResourceID.h"
#include "RenderGraphResourceProperties.h"

#include <string>
#include <unordered_map>

namespace GHL {

	class Heap;

}

namespace Renderer {

	class Buffer;
	class Texture;

	class RenderGraphResource {
	public:
		struct SubresourceRequestedInfo {
			GHL::EResourceState expectedStates{ GHL::EResourceState::Common };
		};

		struct PassRequestedInfo {
		public:
			std::vector<SubresourceRequestedInfo> subresourceRequestedInfos;
		};

	public:
		/*
		* Called When Create Internal Pipeline Resource
		*/
		RenderGraphResource(const GHL::Device* device, const std::string& name);

		/*
		* Called When Import External Pipeline Texture Resource
		*/
		RenderGraphResource(const std::string& name, Texture* resource);

		/*
		* Called When Import External Pipeline Buffer Resource
		*/
		RenderGraphResource(const std::string& name, Buffer*  resource);
		
		~RenderGraphResource() = default;

		/*
		* Build Resource Format For Internal Pipeline Resource to Get D3D12_RESOURCE_DESC & AllocationInfo
		*/
		void BuildResourceFormat();

		inline const auto& GetRequiredMemory() const { return resourceFormat.GetSizeInBytes(); }

		/*
		* Set Subresource Requested Info For Target Pass
		*/
		void SetSubresourceRequestedInfo(uint64_t passNodeIndex, uint32_t subresourceIndex, GHL::EResourceState subresourceExpectedStates);

	public:
		RenderGraphResourceID resourceID;

		bool aliased { false };
		bool imported{ false };

		GHL::Heap*	heap{ nullptr };
		size_t		heapOffset{ 0u };

		Texture* texture{ nullptr };
		Buffer*  buffer { nullptr };

		ResourceFormat		  resourceFormat;
		NewResourceProperties newResourceProperties;

		GHL::EResourceState initialStates { GHL::EResourceState::Common };
		GHL::EResourceState expectedStates{ GHL::EResourceState::Common };

		std::unordered_map<uint64_t, PassRequestedInfo> requestedInfoPerPass;

	};

}