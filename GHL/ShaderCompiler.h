#pragma once
#include "Shader.h"

namespace GHL {
    class CustomIncludeHandler : public IDxcIncludeHandler {
    public:
        CustomIncludeHandler(const std::filesystem::path& rootPath, IDxcLibrary* library);
        ~CustomIncludeHandler() = default;

        virtual HRESULT STDMETHODCALLTYPE LoadSource(_In_ LPCWSTR pFilename, _COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource) override;
        virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override;

        virtual ULONG AddRef() override;
        virtual ULONG Release() override;

    private:
        std::vector<std::string> mReadFileList;
        std::filesystem::path mRootPath;
        IDxcLibrary* mLibrary;
        ULONG mRefCount;

    public:
        inline const auto& AllReadFileRelativePaths() const { return mReadFileList; }
    };

    struct ShaderCompilationResult {
        Shader CompiledShader;
        std::vector<std::string> CompiledFileRelativePaths;
    };

	class ShaderCompiler {
    public:
        struct ShaderCompilationResult {
            Shader CompiledShader;
            std::vector<std::string> CompiledFileRelativePaths;
        };

        ShaderCompiler();
        ~ShaderCompiler() = default;

        ShaderCompilationResult CompileShader(const ShaderDesc& desc);

    private:
        struct BlobCompilationResult {
            Microsoft::WRL::ComPtr<IDxcBlob> Blob;
            Microsoft::WRL::ComPtr<IDxcBlob> PDBBlob;
            std::vector<std::string> CompiledFileRelativePaths;
            std::string DebugName;
        };

        BlobCompilationResult CompileBlob(const ShaderDesc& desc);

        Microsoft::WRL::ComPtr<IDxcLibrary> mLibrary;
        Microsoft::WRL::ComPtr<IDxcCompiler2> mCompiler;
	};
}