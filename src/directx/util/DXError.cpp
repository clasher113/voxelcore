#ifdef USE_DIRECTX
#include "DXError.hpp"

#include <comdef.h>

bool DXError::checkError(HRESULT errorCode, const std::wstring_view& message, bool fatal) {
	if (SUCCEEDED(errorCode)) return true;
	_com_error error(errorCode);
	std::wstring errorMessage((message.empty() ? L"" : message.data() + std::wstring(L"\n")) + error.ErrorMessage());
	MessageBoxW(NULL, errorMessage.c_str(), L"Error", MB_ICONERROR);
	//if (fatal) exit(1);
	return false;
}

void DXError::throwError(HRESULT errorCode, const std::wstring_view& message, bool fatal) {
	if (SUCCEEDED(errorCode)) return;
	MessageBoxW(NULL, message.data(), L"Error", MB_ICONERROR);
	//if (fatal) exit(1);
}

#endif // USE_DIRECTX