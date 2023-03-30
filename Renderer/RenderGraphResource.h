#pragma once
#include "ResourceFormat.h"
#include <string>
#include <unordered_map>

namespace Renderer {

	class Buffer;
	class Texture;

	/*
	* ��������
	*/
	struct NewTextureProperties {
		GHL::ETextureDimension dimension = GHL::ETextureDimension::Texture2D;
		uint32_t               width = 0u;
		uint32_t               height = 0u;
		uint32_t               depth = 1u;
		uint32_t               arraySize = 1u;
		uint32_t               mipLevals = 1u;
		uint32_t               sampleCount = 1u;
		DXGI_FORMAT            format = DXGI_FORMAT_UNKNOWN;
		GHL::EResourceUsage    usage = GHL::EResourceUsage::Default;
		GHL::ETextureMiscFlag  miscFlag = GHL::ETextureMiscFlag::None;
		D3D12_CLEAR_VALUE      clearValue{};
	};

	/*
	* ��������
	*/
	struct NewBufferProperties {
		uint32_t               stride = 0u;
		size_t                 size = 0u;
		DXGI_FORMAT            format = DXGI_FORMAT_UNKNOWN;
		GHL::EResourceUsage    usage = GHL::EResourceUsage::Upload;
		GHL::EBufferMiscFlag   miscFlag = GHL::EBufferMiscFlag::None;
	};

	/*
	* ��ɫ������Դ�ķ��ʱ�ʶ��
	*/
	enum class ShaderAccessFlag {
		NonPixelShader, // ��������ɫ������
		PixelShader,    // ������ɫ������
		AnyShader       // ������ɫ�����ɷ���
	};

	using NewResourceProperties = std::variant<NewTextureProperties, NewBufferProperties>;

	class RenderGraphResource {
	public:
		using ResourceUsageTimeline = std::pair<uint64_t, uint64_t>;

	public:
		RenderGraphResource(const GHL::Device* device, const std::string& name);
		RenderGraphResource(const std::string& name, Texture* resource);
		RenderGraphResource(const std::string& name, Buffer*  resource);
		~RenderGraphResource() = default;

		/*
		* ����ResourceFormat����ȡD3D12Resource �� �Դ���Ϣ
		*/
		void BuildResourceFormat();

		void StartTimeline(uint64_t nodeGlobalExecutionIndex);

		void UpdateTimeline(uint64_t nodeGlobalExecutionIndex);

		void SetNewTextureProperties(const NewTextureProperties& properties);

		void SetNewBufferProperties(const NewBufferProperties& properties);

		void SetInitialStates(GHL::EResourceState states);

		void SetExpectedStates(uint64_t nodeIndex, GHL::EResourceState states);

	private:
		const GHL::Device* mDevice{ nullptr };
		std::string mResName; // ��Դ����

		Texture* mTexture{ nullptr };
		Buffer*  mBuffer { nullptr };

		// ��������Դ������Ϣ

		bool mImported{ false };         // ��Դ�Ƿ��������ⲿ����
		bool mAliase{ false };           // ��Դ�Ƿ��Ǳ���
		ResourceUsageTimeline mTimeline; // ��Դ����������
		NewResourceProperties mNewResourceProperties;
		uint64_t mHeapOffset{ 0u };
		GHL::EResourceState mInitialStates{ GHL::EResourceState::Common };
		GHL::EResourceState mExpectedStates{ GHL::EResourceState::Common };
		std::unordered_map<uint64_t, GHL::EResourceState> mExpectedStatesPerPass; // ����Pass�Ը���Դ��Ҫ��

		ResourceFormat mResourceFormat;
	};

}