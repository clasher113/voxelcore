#ifndef DX_ERROR_HPP
#define DX_ERROR_HPP

#ifdef _DEBUG
#define CHECK_ERROR3(HR, ERRMSG, FATAL) if (FAILED(HR)) DXError::checkError(HR, __FILE__, __FUNCTION__, __LINE__, ERRMSG, FATAL)
#define PRINT_ERROR2(HR, ERRMSG) if (FAILED(HR)) DXError::printError(HR, __FILE__, __FUNCTION__, __LINE__, ERRMSG)
#else
#define CHECK_ERROR3(HR, ERRMSG, FATAL) HR
#define PRINT_ERROR2(HR, ERRMSG) HR
#endif // _DEBUG
#define CHECK_ERROR2(HR, ERRMSG) CHECK_ERROR3(HR, ERRMSG, true)
#define CHECK_ERROR1(HR) CHECK_ERROR3(HR, L"", true)
#define PRINT_ERROR1(HR) PRINT_ERROR2(HR, L"")

#include <d3d11_1.h>
#include <string>

class DXError {
public:
	static void printError(HRESULT errorCode, const std::string& file, const std::string& function, int line,
		const std::wstring& message = L"");
	static void checkError(HRESULT errorCode, const std::string& file, const std::string& function, int line,
		const std::wstring& message = L"", bool fatal = true);
	static void throwWarn(const std::wstring& message);
private:
	static std::wstring constructMessage(HRESULT errorCode, const std::string& file, const std::string& function, int line,
		const std::wstring& message);
};


#endif // !DX_ERROR_HPP