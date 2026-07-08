#pragma once

#ifdef _DEBUG
#define CHECK_ERROR3(HR, ERRMSG, FATAL) if (FAILED(HR)) Error::checkError(HR, __FILE__, __FUNCTION__, __LINE__, ERRMSG, FATAL)
#define PRINT_ERROR2(HR, ERRMSG) if (FAILED(HR)) Error::printError(HR, __FILE__, __FUNCTION__, __LINE__, ERRMSG)
#else
#define CHECK_ERROR3(HR, ERRMSG, FATAL) HR
#define PRINT_ERROR2(HR, ERRMSG) HR
#endif // _DEBUG
#define CHECK_ERROR2(HR, ERRMSG) CHECK_ERROR3(HR, ERRMSG, true)
#define CHECK_ERROR1(HR) CHECK_ERROR3(HR, L"", true)
#define PRINT_ERROR1(HR) PRINT_ERROR2(HR, L"")

#define NOMINMAX
#include <d3d11_1.h>
#include <string>

class Error {
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