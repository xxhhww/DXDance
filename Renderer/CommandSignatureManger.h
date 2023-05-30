#pragma once
#include "GHL/CommandSignature.h"
#include <functional>
#include <unordered_map>

namespace Renderer {

	class CommandSignatureManger {
	public:
		using CommandSignatureConfigurator = std::function<void(GHL::CommandSignature&)>;

	public:
		CommandSignatureManger(const GHL::Device* device);
		~CommandSignatureManger() = default;

		void CreateCommandSignature(const std::string& name, const CommandSignatureConfigurator& configurator);

		inline auto* GetD3DCommandSignature(const std::string& name) const { return mCommandSignatures.at(name)->D3DCommandSignature(); }

	private:
		const GHL::Device* mDevice{ nullptr };
		std::unordered_map<std::string, std::unique_ptr<GHL::CommandSignature>> mCommandSignatures;
	};

}
