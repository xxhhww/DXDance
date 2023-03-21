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
		* @Param relativePath: ���·��(Assets/... ���� Engine/...)
		* @Param stage: ��ɫ������(VS...)
		*/
		GHL::Shader* GetShader(const std::string& relativePath, GHL::EShaderStage stage);

		/*
		* �ض�λ
		*/
		void Repath(const std::string& oldPath, const std::string& newPath);

		/*
		* �ر���
		*/
		void Recompile(const std::string& relativePath, GHL::EShaderStage stage);

	private:
		/*
		* ���벢������ɫ��
		*/
		GHL::Shader* CompileAndCacheShader(const std::string& relativePath, GHL::EShaderStage stage);

	private:
		struct CompiledFile {
			std::string path; // �ļ�·��
			std::unordered_map<GHL::EShaderStage, std::unique_ptr<GHL::Shader>> shaders; // ��ɫ���ļ��������Shader
			std::unordered_set<std::string> includedFilePaths; // Include�ļ�
		};

	private:
		const std::string mProjectPath;
		bool mBuildDebugShaders{ true };
		bool mSeparatePDBFiles{ false };

		GHL::ShaderCompiler mCompiler;

		std::unordered_map<std::string, CompiledFile> mCompiledFileMap;

	};

}