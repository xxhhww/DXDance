#include "ShaderManger.h"

namespace Renderer {

	ShaderManger::ShaderManger(const std::string& projectPath, bool buildDebugShaders, bool separatePDBFiles)
	: mProjectPath(projectPath)
	, mBuildDebugShaders(buildDebugShaders)
	, mSeparatePDBFiles(separatePDBFiles) 
	, mCompiler() {}

	ShaderManger::~ShaderManger() {
		for (const auto& pair : mCompiledFileMap) {
			delete pair.second;
		}
		mCompiledFileMap.clear();
	}

	GHL::Shader* ShaderManger::GetShader(const std::string& relativePath, GHL::EShaderStage stage) {
		if (mCompiledFileMap.find(relativePath) == mCompiledFileMap.end()) {
			CompileAndCacheShader(relativePath, stage);
		}
		auto& shaders = mCompiledFileMap.at(relativePath)->shaders;
		
		if (shaders.find(stage) == shaders.end()) {
			CompileAndCacheShader(relativePath, stage);
		}

		if (shaders.find(stage) == shaders.end()) {
			return nullptr;
		}

		return shaders.at(stage).get();
	}

	void ShaderManger::Repath(const std::string& oldPath, const std::string& newPath) {
		auto it = mCompiledFileMap.find(oldPath);

		if (it == mCompiledFileMap.end()) {
			return;
		}

		auto* targetFile = it->second;
		targetFile->path = newPath;

		mCompiledFileMap.erase(it);

		mCompiledFileMap[newPath] = targetFile;
	}

	void ShaderManger::Recompile(const std::string& relativePath, GHL::EShaderStage stage) {
		if (mCompiledFileMap.find(relativePath) == mCompiledFileMap.end()) {
			return;
		}

		auto& shaders = mCompiledFileMap.at(relativePath)->shaders;
		if (shaders.find(stage) == shaders.end()) {
			return;
		}

		shaders.at(stage) = nullptr;
		

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

		if (mCompiledFileMap.find(relativePath) == mCompiledFileMap.end()) {
			mCompiledFileMap[relativePath] = new CompiledFile();
		}

		CompiledFile* compiledFile = mCompiledFileMap[relativePath];
		compiledFile->shaders[stage] = std::make_unique<GHL::Shader>(compilationResult.CompiledShader);


		for (auto& includedFilePath : compilationResult.CompiledFileRelativePaths) {
			if (compiledFile->includedFilePaths.find(includedFilePath) == compiledFile->includedFilePaths.end()) {
				compiledFile->includedFilePaths.insert(includedFilePath);
			}
		}

		return compiledFile->shaders[stage].get();
	}

	void ShaderManger::Delete(const std::string& relativePath) {
		auto it = mCompiledFileMap.find(relativePath);

		if (it == mCompiledFileMap.end()) {
			return;
		}

		delete it->second;

		mCompiledFileMap.erase(it);
	}

}