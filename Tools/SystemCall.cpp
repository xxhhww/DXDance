#include "SystemCall.h"
#include "StrUtil.h"
#include <Windows.h>

namespace Tool {

	void SystemCalls::ShowInExplorer(const std::string& p_path) {
		ShellExecuteA(NULL, "open", StrUtil::MakeWindowsStyle(p_path).c_str(), NULL, NULL, SW_SHOWNORMAL);
	}

	void SystemCalls::OpenFile(const std::string& p_file, const std::string& p_workingDir) {
		ShellExecuteA(NULL, "open", StrUtil::MakeWindowsStyle(p_file).c_str(), NULL,
			p_workingDir.empty() ? NULL : StrUtil::MakeWindowsStyle(p_workingDir).c_str(),
			SW_SHOWNORMAL);
	}

	void SystemCalls::EditFile(const std::string& p_file) {
		ShellExecuteW(NULL, NULL, std::wstring(p_file.begin(), p_file.end()).c_str(), NULL, NULL, SW_NORMAL);
	}

	void SystemCalls::OpenURL(const std::string& p_url) {
		ShellExecute(0, 0, StrUtil::UTF8ToWString(p_url).c_str(), 0, 0, SW_SHOW);
	}
}