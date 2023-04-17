#include "ShaderCompiler.h"
#include "Tools/Assert.h"
#include "Tools/StrUtil.h"

namespace GHL {

	char const* shaderDirectory = "E:/MyProject/DXDance/Shader/";

    ShaderCompiler::ShaderCompiler() {
        HRASSERT(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(mLibrary.GetAddressOf())));
        HRASSERT(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(mCompiler.GetAddressOf())));
    }

    ShaderCompilationResult ShaderCompiler::CompileShader(const ShaderDesc& desc) const {
		ShaderCompilationResult output;

		uint32_t codePage = CP_UTF8;
		Microsoft::WRL::ComPtr<IDxcBlobEncoding> sourceBlob;

		std::wstring shaderFile = Tool::StrUtil::UTF8ToWString(desc.file);
		HRASSERT(mLibrary->CreateBlobFromFile(shaderFile.data(), &codePage, sourceBlob.GetAddressOf()));
		
		std::wstring name = Tool::StrUtil::UTF8ToWString(Tool::StrUtil::RemoveExtension(desc.file));
		std::wstring dir = Tool::StrUtil::UTF8ToWString(shaderDirectory);
		std::wstring path = Tool::StrUtil::UTF8ToWString(Tool::StrUtil::GetBasePath(desc.file));

		std::wstring target = Tool::StrUtil::UTF8ToWString(GenProfileString(desc.stage, desc.model));
		std::wstring entryPoint = Tool::StrUtil::UTF8ToWString(desc.entryPoint);

		std::vector<wchar_t const*> compileArgs{};
		compileArgs.push_back(name.c_str());

		if (HasAnyFlag(desc.compileFlag, EShaderCompileFlag::Debug)) {
			compileArgs.push_back(DXC_ARG_DEBUG);
			compileArgs.push_back(L"-Qembed_debug");
		}
		else {
			compileArgs.push_back(L"-Qstrip_debug");
			compileArgs.push_back(L"-Qstrip_reflect");
		}

		if (HasAnyFlag(desc.compileFlag, EShaderCompileFlag::DisableOptimization)) {
			compileArgs.push_back(DXC_ARG_SKIP_OPTIMIZATIONS);
		}
		else {
			compileArgs.push_back(DXC_ARG_OPTIMIZATION_LEVEL3);
		}

		compileArgs.push_back(L"-HV 2021");

		compileArgs.push_back(L"-E");
		compileArgs.push_back(entryPoint.c_str());
		compileArgs.push_back(L"-T");
		compileArgs.push_back(target.c_str());

		compileArgs.push_back(L"-I");
		compileArgs.push_back(dir.c_str());
		compileArgs.push_back(L"-I");
		compileArgs.push_back(path.c_str());

		std::vector<std::wstring> macros;
		macros.reserve(desc.macros.size());
		for (auto const& macro : desc.macros) {
			std::wstring name = Tool::StrUtil::UTF8ToWString(macro.name);
			std::wstring value = Tool::StrUtil::UTF8ToWString(macro.value);
			compileArgs.push_back(L"-D");
			if (macro.value.empty())
				macros.push_back(name + L"=1");
			else
				macros.push_back(name + L"=" + value);
			compileArgs.push_back(macros.back().c_str());
		}

		DxcBuffer source_buffer;
		source_buffer.Ptr = sourceBlob->GetBufferPointer();
		source_buffer.Size = sourceBlob->GetBufferSize();
		source_buffer.Encoding = 0;
		
		// CustomIncludeHandler custom_include_handler{};

		Microsoft::WRL::ComPtr<IDxcResult> result;
		HRASSERT(mCompiler->Compile(
			&source_buffer,
			compileArgs.data(), (uint32_t)compileArgs.size(),
			nullptr,
			IID_PPV_ARGS(result.GetAddressOf())));

		Microsoft::WRL::ComPtr<IDxcBlobUtf8> errors;
		if (SUCCEEDED(result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(errors.GetAddressOf()), nullptr))) {
			if (errors && errors->GetStringLength() > 0) {
				ASSERT_FORMAT(false, errors->GetStringPointer());
			}
		}
		Microsoft::WRL::ComPtr<IDxcBlob> blob;
		HRASSERT(result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(blob.GetAddressOf()), nullptr));

		Microsoft::WRL::ComPtr<IDxcBlob> hash;
		if (SUCCEEDED(result->GetOutput(DXC_OUT_SHADER_HASH, IID_PPV_ARGS(hash.GetAddressOf()), nullptr))) {
			DxcShaderHash* hash_buf = (DxcShaderHash*)hash->GetBufferPointer();
			memcpy(output.shaderHash, hash_buf->HashDigest, sizeof(uint64_t) * 2);
		}

		output.compiledShader.SetDesc(desc);
		output.compiledShader.SetBytecode(blob->GetBufferPointer(), blob->GetBufferSize());
		// output.includes = std::move(custom_include_handler.include_files);
		output.includes.push_back(desc.file);

		return output;
    }

}