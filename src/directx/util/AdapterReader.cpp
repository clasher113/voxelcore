#ifdef USE_DIRECTX
#include "AdapterReader.hpp"
#include "DXError.hpp"

std::vector<AdapterData> AdapterReader::GetAdapters() {
	if (!adapters.empty()) //If already initialized
		return adapters;

	Microsoft::WRL::ComPtr<IDXGIFactory> pFactory;

	CHECK_ERROR2(CreateDXGIFactory(IID_PPV_ARGS(pFactory.GetAddressOf())),
		L"Failed to create DXGIFactory for enumerating adapters.");

	Microsoft::WRL::ComPtr<IDXGIAdapter> pAdapter;
	UINT index = 0;
	while (SUCCEEDED(pFactory->EnumAdapters(index, pAdapter.GetAddressOf()))) {
		adapters.emplace_back(pAdapter);
		index += 1;
	}
	return adapters;
}

AdapterData::AdapterData(Microsoft::WRL::ComPtr<IDXGIAdapter> pAdapter) :
	m_adapter(pAdapter)
{
	CHECK_ERROR2(m_adapter->GetDesc(&m_description),
		L"Failed to Get Description for IDXGIAdapter.");
}

#endif // USE_DIRECTX