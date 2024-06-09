#ifdef USE_DIRECTX
#include "DXError.hpp"
#include "../window/DXDevice.hpp"
#include "../../util/stringutil.h"

#include <comdef.h>

static const std::wstring continueQuestion(L"Do you want to continue?");
static const std::wstring feautureWarn(L"Some features may not work.");

void DXError::checkError(HRESULT errorCode, const std::string& file, const std::string& function, int line, const std::wstring& message, bool fatal) {
	_com_error error(errorCode);
	std::wstring errorMessage;
	if (!message.empty()) errorMessage += message + L"\n";
	errorMessage += error.ErrorMessage() + std::wstring(L"\n");
	errorMessage += L"\nFile: " + util::str2wstr_utf8(file);
	errorMessage += L"\nFunction: " + util::str2wstr_utf8(function);
	errorMessage += L"\nLine: " + std::to_wstring(line);

	if (fatal) {
		MessageBoxW(DXDevice::getWindowHandle(), errorMessage.c_str(), L"Fatal Error", MB_OK | MB_ICONERROR);
		exit(errorCode);
	}
	else {
		errorMessage += L"\n" + feautureWarn + L"\n" + continueQuestion;
		int dr = MessageBoxW(DXDevice::getWindowHandle(), errorMessage.c_str(), L"Error", MB_YESNO | MB_ICONERROR);
		if (dr == IDNO) exit(errorCode);
	}
}

void DXError::throwWarn(const std::wstring& message) {
	int dr = MessageBoxW(DXDevice::getWindowHandle(), (message + L"\n" + continueQuestion).data(), L"Warning", MB_YESNO | MB_ICONWARNING);
	if (dr == IDNO) exit(-1);
}

#endif // USE_DIRECTX