#include "ShaderCompiler.h"

#include "Tools/Assert.h"
#include "Tools/StrUtil.h"

#include <fstream>
#include <filesystem>

namespace GHL {

	char const* shaderDirectory = "E:/MyProject/DXDance/Shader/";

	class CustomIncludeHandler : public IDxcIncludeHandler {
	public:
		CustomIncludeHandler(IDxcIncludeHandler* includeHandler, IDxcUtils* utils) 
		: mIncludeHandler(includeHandler)
		, mUtils(utils) {}

		HRESULT STDMETHODCALLTYPE LoadSource(_In_ LPCWSTR pFilename, _COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource) override {
			Microsoft::WRL::ComPtr<IDxcBlobEncoding> encoding;
			std::string include_file = Tool::StrUtil::WStringToUTF8(pFilename);
			if (!std::filesystem::exists(include_file)) {
				*ppIncludeSource = nullptr;
				return E_FAIL;
			}

			bool already_included = false;
			for (auto const& includedFile : includeFiles) {
				if (include_file == includedFile) {
					already_included = true;
					break;
				}
			}

			if (already_included) {
				static const char nullStr[] = " ";
				mUtils->CreateBlob(nullStr, ARRAYSIZE(nullStr), CP_UTF8, encoding.GetAddressOf());
				*ppIncludeSource = encoding.Detach();
				return S_OK;
			}

			std::wstring winclude_file = Tool::StrUtil::UTF8ToWString(include_file);
			HRESULT hr = mUtils->LoadFile(winclude_file.c_str(), nullptr, encoding.GetAddressOf());
			if (SUCCEEDED(hr)) {
				includeFiles.push_back(include_file);
				*ppIncludeSource = encoding.Detach();
				return S_OK;
			}
			else *ppIncludeSource = nullptr;
			return E_FAIL;
		}
		
		HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) override {
			return mIncludeHandler->QueryInterface(riid, ppvObject);
		}

		ULONG STDMETHODCALLTYPE AddRef(void) override { return 1; }
		ULONG STDMETHODCALLTYPE Release(void) override { return 1; }

	public:
		IDxcIncludeHandler* mIncludeHandler{ nullptr };
		IDxcUtils* mUtils{ nullptr };
		std::vector<std::string> includeFiles;
	};

    ShaderCompiler::ShaderCompiler() {
        HRASSERT(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(mLibrary.GetAddressOf())));
        HRASSERT(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(mCompiler.GetAddressOf())));
		HRASSERT(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(mUtils.GetAddressOf())));
		HRASSERT(mLibrary->CreateIncludeHandler(mIncludeHandler.GetAddressOf()));
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
		
		CustomIncludeHandler customIncludeHandler{ mIncludeHandler.Get(), mUtils.Get() };

		Microsoft::WRL::ComPtr<IDxcResult> result;
		HRASSERT(mCompiler->Compile(
			&source_buffer,
			compileArgs.data(), (uint32_t)compileArgs.size(),
			&customIncludeHandler,
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

		Microsoft::WRL::ComPtr<IDxcBlob> pdbBlob;
		Microsoft::WRL::ComPtr<IDxcBlobUtf16> pDebugDataPath;
		if (SUCCEEDED(result->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(pdbBlob.GetAddressOf()), pDebugDataPath.GetAddressOf()))) {
			int i = 32;
		}

		output.compiledShader.SetDesc(desc);
		output.compiledShader.SetBytecode(blob->GetBufferPointer(), blob->GetBufferSize());
		output.includes = std::move(customIncludeHandler.includeFiles);
		output.includes.push_back(desc.file);

		return output;
    }

}