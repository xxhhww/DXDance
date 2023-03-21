#include "ShaderManger.h"

namespace Renderer {

	ShaderManger::ShaderManger(const std::string& projectPath, bool buildDebugShaders, bool separatePDBFiles)
	: mProjectPath(projectPath)
	, mBuildDebugShaders(buildDebugShaders)
	, mSeparatePDBFiles(separatePDBFiles) 
	, mCompiler() {}

	GHL::Shader* ShaderManger::GetShader(const std::string& relativePath, GHL::EShaderStage stage) {
		if (mCompiledFileMap.find(relativePath) == mCompiledFileMap.end()) {
			CompileAndCacheShader(relativePath, stage);
		}
		auto& shaders = mCompiledFileMap.at(relativePath).shaders;
		
		if (shaders.find(stage) == shaders.end()) {
			CompileAndCacheShader(relativePath, stage);
		}

		if (shaders.find(stage) == shaders.end()) {
			return nullptr;
		}

		return shaders.at(stage).get();
	}

	void ShaderManger::Repath(const std::string& oldPath, const std::string& newPath) {

	}

	void ShaderManger::Recompile(const std::string& relativePath, GHL::EShaderStage stage) {
		if (mCompiledFileMap.find(relativePath) == mCompiledFileMap.end()) {
			return;
		}

		CompileAndCacheShader(relativePath, stage);
	}

	GHL::Shader* ShaderManger::CompileAndCacheShader(const std::string& relativePath, GHL::EShaderStage stage) {
		const std::string fullPath = mProjectPath + relativePath;
		GHL::ShaderDesc desc{};
		desc.stage = stage;
		desc.path = fullPath;
		desc.debugBuild = mBuildDebugShaders;
		desc.separatePDB = mSeparatePDBFiles;

		GHL::ShaderCompiler::ShaderCompilationResult compilationResult = mCompiler.CompileShader(desc);

		if (!compilationResult.CompiledShader.GetBlob()) {
			return nullptr;
		}

		// Associate shader with a file it was loaded from and its entry point name
		CompiledFile& compiledFile = mCompiledFileMap[relativePath];
		compiledFile.shaders[stage] = std::make_unique<GHL::Shader>(compilationResult.CompiledShader);


		for (auto& shaderFilePath : compilationResult.CompiledFileRelativePaths) {
			// Associate every file that took place in compilation with the root file that has shader's entry point
			compiledFile.includedFilePaths.insert(shaderFilePath);
		}

		return compiledFile.shaders[stage].get();
	}

}