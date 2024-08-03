#ifdef USE_DIRECTX
#include "AdapterReader.hpp"
#include "DXError.hpp"

const std::vector<AdapterData>& AdapterReader::GetAdapters() {
	if (!s_m_adapters.empty()) //If already initialized
		return s_m_adapters;

	Microsoft::WRL::ComPtr<IDXGIFactory> pFactory;

	CHECK_ERROR2(CreateDXGIFactory(IID_PPV_ARGS(pFactory.GetAddressOf())),
		L"Failed to create DXGIFactory for enumerating adapters.");

	Microsoft::WRL::ComPtr<IDXGIAdapter> pAdapter;
	UINT index = 0;
	while (SUCCEEDED(pFactory->EnumAdapters(index, pAdapter.GetAddressOf()))) {
		s_m_adapters.emplace_back(pAdapter);
		index += 1;
	}
	return s_m_adapters;
}

AdapterData::AdapterData(Microsoft::WRL::ComPtr<IDXGIAdapter> pAdapter) :
	m_adapter(pAdapter),
	m_vendor("Undefined")
{
	CHECK_ERROR2(m_adapter->GetDesc(&m_description),
		L"Failed to Get Description for IDXGIAdapter.");

	switch (m_description.VendorId) {
		case 0x10DE:
			m_vendor = "Nvidia";
			break;
		case 0x1002:
		case 0x1022:
			m_vendor = "ATI Technologies";
			break;
		case 0x163C:
		case 0x8086:
		case 0x8087:
			m_vendor = "Intel";
			break;
		case 0x1414:
			m_vendor = "Microsoft Basic Render Driver";
			break;
	}
}

#endif // USE_DIRECTX