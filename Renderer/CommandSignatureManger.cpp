#include "CommandSignatureManger.h"
#include "Tools/Assert.h"

namespace Renderer {

	CommandSignatureManger::CommandSignatureManger(const GHL::Device* device)
	: mDevice(device) {}

	void CommandSignatureManger::CreateCommandSignature(const std::string& name, const CommandSignatureConfigurator& configurator) {
		ASSERT_FORMAT(mCommandSignatures.find(name) == mCommandSignatures.end(), "Command Signature already exists");

		std::unique_ptr<GHL::CommandSignature> commandSignature = std::make_unique<GHL::CommandSignature>(mDevice);

		configurator(*commandSignature);

		commandSignature->Compile();

		mCommandSignatures[name] = std::move(commandSignature);
	}

}