#pragma once
#include "GHL/Shader.h"
#include "GHL/ShaderCompiler.h"
#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace Renderer {

	class ShaderManger {
	public:
		ShaderManger(const std::string& projectPath, bool buildDebugShaders = true, bool separatePDBFiles = false);
		~ShaderManger() = default;

		/*
		* @Param relativePath: 相对路径(Assets/... 或者 Engine/...)
		* @Param stage: 着色器类型(VS...)
		*/
		GHL::Shader* GetShader(const std::string& relativePath, GHL::EShaderStage stage);

		/*
		* 重定位
		*/
		void Repath(const std::string& oldPath, const std::string& newPath);

		/*
		* 重编译
		*/
		void Recompile(const std::string& relativePath, GHL::EShaderStage stage);

	private:
		/*
		* 编译并缓存着色器
		*/
		GHL::Shader* CompileAndCacheShader(const std::string& relativePath, GHL::EShaderStage stage);

	private:
		struct CompiledFile {
			std::string path; // 文件路径
			std::unordered_map<GHL::EShaderStage, std::unique_ptr<GHL::Shader>> shaders; // 着色器文件编译出的Shader
			std::unordered_set<std::string> includedFilePaths; // Include文件
		};

	private:
		const std::string mProjectPath;
		bool mBuildDebugShaders{ true };
		bool mSeparatePDBFiles{ false };

		GHL::ShaderCompiler mCompiler;

		std::unordered_map<std::string, CompiledFile> mCompiledFileMap;

	};

}