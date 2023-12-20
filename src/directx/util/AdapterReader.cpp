#ifdef USE_DIRECTX
#include "AdapterReader.hpp"
#include "DXError.hpp"

#include <wrl/client.h>

std::vector<AdapterData> AdapterReader::adapters;

std::vector<AdapterData> AdapterReader::GetAdapters() {
	if (!adapters.empty()) //If already initialized
		return adapters;

	Microsoft::WRL::ComPtr<IDXGIFactory> pFactory;

	DXError::checkError(CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(pFactory.GetAddressOf())),
		L"Failed to create DXGIFactory for enumerating adapters.");

	IDXGIAdapter* pAdapter;
	UINT index = 0;
	while (SUCCEEDED(pFactory->EnumAdapters(index, &pAdapter))) {
		adapters.push_back(pAdapter);
		index += 1;
	}
	return adapters;
}

AdapterData::AdapterData(IDXGIAdapter* pAdapter) {
	this->pAdapter = pAdapter;
	DXError::checkError(pAdapter->GetDesc(&this->description),
		L"Failed to Get Description for IDXGIAdapter.");
}

#endif // USE_DIRECTX