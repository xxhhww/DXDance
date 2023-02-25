#include "OpenFileDialog.h"
#include <Windows.h>

namespace Windows {
	OpenFileDialog::OpenFileDialog(const std::string& p_dialogTitle) : FileDialog(GetOpenFileNameW, p_dialogTitle)
	{
	}

	void OpenFileDialog::AddFileType(const std::string& p_label, const std::string& p_filter)
	{
		m_filter += p_label + '\0' + p_filter + '\0';
	}
}