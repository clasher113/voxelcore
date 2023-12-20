#ifndef DX_ERROR_HPP
#define DX_ERROR_HPP

#include <d3d11_1.h>
#include <string_view>

class DXError {
public:
	static bool checkError(HRESULT errorCode, const std::wstring_view& message = L"", bool fatal = true);
	static void throwError(HRESULT errorCode, const std::wstring_view& message = L"", bool fatal = true);
};

#endif // !DX_ERROR_HPP