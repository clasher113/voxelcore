#ifdef USE_DIRECTX
#include "Error.hpp"

#include "directx/window/Device.hpp"
#include "util/stringutil.hpp"

#include <iostream>
#include <comdef.h>

static const std::wstring continueQuestion(L"Do you want to continue?");
static const std::wstring feautureWarn(L"Some features may not work.");

void Error::printError(HRESULT errorCode, const std::string& file, const std::string& function, int line, const std::wstring& message) {
	std::wcout << constructMessage(errorCode, file, function, line, message) << std::endl;
}

void Error::checkError(HRESULT errorCode, const std::string& file, const std::string& function, int line, const std::wstring& message, bool fatal) {

	std::wstring errorMessage = constructMessage(errorCode, file, function, line, message);

	if (fatal) {
		MessageBoxW(Device::getWindowHandle(), errorMessage.c_str(), L"Fatal Error", MB_OK | MB_ICONERROR);
		exit(errorCode);
	}
	else {
		errorMessage += L"\n" + feautureWarn + L"\n" + continueQuestion;
		int dr = MessageBoxW(Device::getWindowHandle(), errorMessage.c_str(), L"Error", MB_YESNO | MB_ICONERROR);
		if (dr == IDNO) exit(errorCode);
	}
}

void Error::throwWarn(const std::wstring& message) {
	int dr = MessageBoxW(Device::getWindowHandle(), (message + L"\n" + continueQuestion).data(), L"Warning", MB_YESNO | MB_ICONWARNING);
	if (dr == IDNO) exit(-1);
}

std::wstring Error::constructMessage(HRESULT errorCode, const std::string& file, const std::string& function, int line, const std::wstring& message) {
	_com_error error(errorCode);
	std::wstring errorMessage;
	if (!message.empty()) errorMessage += message + L"\n";
	errorMessage += util::str2wstr_utf8(error.ErrorMessage()) + L'\n';
	errorMessage += L"\nFile: " + util::str2wstr_utf8(file);
	errorMessage += L"\nFunction: " + util::str2wstr_utf8(function);
	errorMessage += L"\nLine: " + std::to_wstring(line);
	return errorMessage;
}

#endif // USE_DIRECTX