#include "ShaderCompiler.h"
#include "Tools/Assert.h"
#include "Tools/StrUtil.h"

namespace GHL {
    CustomIncludeHandler::CustomIncludeHandler(const std::filesystem::path& rootPath, IDxcLibrary* library)
    : mRootPath{ rootPath }, mLibrary{ library } {}

    HRESULT STDMETHODCALLTYPE CustomIncludeHandler::LoadSource(_In_ LPCWSTR pFilename, _COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource)
    {
        std::filesystem::path includePath{ pFilename };

        std::string fileName = includePath.string();

        // Search for the substring in string
        size_t pos = fileName.find("./");

        if (pos != std::string::npos)
        {
            // If found then erase it from string
            fileName.erase(pos, 2);
        }

        mReadFileList.push_back(fileName);

        includePath = mRootPath / pFilename;

        IDxcBlobEncoding* source;
        HRESULT result = mLibrary->CreateBlobFromFile(includePath.wstring().c_str(), nullptr, &source);
        *ppIncludeSource = source;
        return result;
    }

    // Implementing IUnknown
    // https://docs.microsoft.com/en-us/office/client-developer/outlook/mapi/implementing-iunknown-in-c-plus-plus
    //
    HRESULT STDMETHODCALLTYPE CustomIncludeHandler::QueryInterface(REFIID riid, void** ppvObject)
    {
        // Always set out parameter to NULL, validating it first.
        if (!ppvObject) return E_INVALIDARG;

        *ppvObject = NULL;
        if (riid == IID_IUnknown)
        {
            // Increment the reference count and return the pointer.
            *ppvObject = (LPVOID)this;
            AddRef();
            return NOERROR;
        }

        return E_NOINTERFACE;
    }

    ULONG CustomIncludeHandler::AddRef()
    {
        InterlockedIncrement(&mRefCount);
        return mRefCount;
    }

    ULONG CustomIncludeHandler::Release()
    {
        ULONG ulRefCount = InterlockedDecrement(&mRefCount);
        //if (mRefCount == 0) delete this; // Crashes, even though it's from MS docs 
        return ulRefCount;
    }

    ShaderCompiler::ShaderCompiler() {
        HRASSERT(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(mLibrary.GetAddressOf())));
        HRASSERT(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(mCompiler.GetAddressOf())));
    }

    ShaderCompiler::ShaderCompilationResult ShaderCompiler::CompileShader(const ShaderDesc& desc) {
        BlobCompilationResult blobCompilationResult = CompileBlob(desc);
        ShaderCompilationResult shaderCompilationResult{ Shader{ blobCompilationResult.Blob, blobCompilationResult.PDBBlob, desc.entryPoint, desc.stage }, blobCompilationResult.CompiledFileRelativePaths };
        return shaderCompilationResult;
    }

    ShaderCompiler::BlobCompilationResult ShaderCompiler::CompileBlob(const ShaderDesc& desc) {
        ASSERT_FORMAT(std::filesystem::exists(desc.path), "Shader file ", desc.path.c_str(), " doesn't exist");

        std::wstring wPath = Tool::StrUtil::UTF8ToWString(desc.path);
        std::wstring wEntryPoint = Tool::StrUtil::UTF8ToWString(desc.entryPoint);
        std::wstring wProfile = Tool::StrUtil::UTF8ToWString(GenProfileString(desc.stage, desc.model));
        LPWSTR suggestedDebugName = nullptr;

        std::vector<std::wstring> arguments;
        arguments.push_back(L"/all_resources_bound");

        if (desc.debugBuild) {
            arguments.push_back(L"/Zi");
            arguments.push_back(L"/Od");

            if (desc.separatePDB) {
                arguments.push_back(L"/Qstrip_debug");
            }
        }

        std::vector<LPCWSTR> argumentPtrs;
        for (auto& argument : arguments) {
            argumentPtrs.push_back(argument.c_str());
        }

        std::vector<DxcDefine> defines;
        for (auto& macro : desc.macros) {
            DxcDefine dxcDefine;
            ZeroMemory(&dxcDefine, sizeof(dxcDefine));
            std::wstring name = Tool::StrUtil::UTF8ToWString(macro.name);
            std::wstring value = Tool::StrUtil::UTF8ToWString(macro.value);

            wmemcpy((wchar_t*)dxcDefine.Name, name.c_str(), name.size());
            wmemcpy((wchar_t*)dxcDefine.Value, value.c_str(), value.size());
            defines.emplace_back(std::move(dxcDefine));
        }

        Microsoft::WRL::ComPtr<IDxcBlob> sourceBlob;
        Microsoft::WRL::ComPtr<IDxcBlob> pdbBlob;
        Microsoft::WRL::ComPtr<IDxcOperationResult> result;

        // TODO 自定义头文件
        CustomIncludeHandler reader{ std::filesystem::path(desc.path), mLibrary.Get() };
        // reader.LoadSource(path.filename().wstring().c_str(), sourceBlob.GetAddressOf());

        // Note: when compiling libraries, entry point and profile are ignored

        if (desc.debugBuild && desc.separatePDB) {
            mCompiler->CompileWithDebug(
                sourceBlob.Get(),                   // Program text
                wPath.c_str(),                      // File name, mostly for error messages
                wEntryPoint.c_str(),                // Entry point function
                wProfile.c_str(),                   // Target profile
                argumentPtrs.data(),                // Compilation arguments
                argumentPtrs.size(),                // Number of compilation arguments
                defines.data(),                     // Name/value defines
                defines.size(),                     // Number of compilation defines
                &reader,                            // Handler for #include directives
                result.GetAddressOf(),              // Compiler output status, buffer, and errors
                &suggestedDebugName,                // Suggested file name for debug blob.
                pdbBlob.GetAddressOf());            // Debug info blob
        }
        else {
            mCompiler->Compile(
                sourceBlob.Get(),                   // Program text
                wPath.c_str(),                      // File name, mostly for error messages
                wEntryPoint.c_str(),                // Entry point function
                wProfile.c_str(),                   // Target profile
                argumentPtrs.data(),                // Compilation arguments
                argumentPtrs.size(),                // Number of compilation arguments
                defines.data(),                     // Name/value defines
                defines.size(),                     // Number of compilation defines
                &reader,                            // Handler for #include directives
                result.GetAddressOf());
        }

        HRESULT hrCompilation{};
        result->GetStatus(&hrCompilation);

        if (SUCCEEDED(hrCompilation)) {
            Microsoft::WRL::ComPtr<IDxcBlob> compiledShaderBlob;
            result->GetResult(compiledShaderBlob.GetAddressOf());
            std::filesystem::path pdbFileName{ suggestedDebugName ? suggestedDebugName : L"" };

            return { compiledShaderBlob, pdbBlob, reader.AllReadFileRelativePaths(), pdbFileName.string() };
        }
        else {
            Microsoft::WRL::ComPtr<IDxcBlobEncoding> printBlob;
            Microsoft::WRL::ComPtr<IDxcBlobEncoding> printBlob16;

            result->GetErrorBuffer(printBlob.GetAddressOf());
            // We can use the library to get our preferred encoding.
            mLibrary->GetBlobAsUtf16(printBlob.Get(), printBlob16.GetAddressOf());
            OutputDebugStringW((LPWSTR)printBlob16->GetBufferPointer());

            return { nullptr, nullptr, {}, "" };
        }
    }

}