#ifndef ADAPTER_READER_HPP
#define ADAPTER_READER_HPP

#include <vector> 
#include <d3d11_1.h>

class AdapterData
{
public:
	AdapterData(IDXGIAdapter* pAdapter);
	IDXGIAdapter* pAdapter = nullptr;
	DXGI_ADAPTER_DESC description;
};

class AdapterReader
{
public:
	static std::vector<AdapterData> GetAdapters();
private:
	static std::vector<AdapterData> adapters;
};

#endif // !ADAPTER_READER_HPPw