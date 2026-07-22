#ifdef USE_DIRECTX
#include "AdapterReader.hpp"

#include "Error.hpp"

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

void AdapterReader::setPreferedAdapter(UINT index) {
	if (index < s_m_adapters.size()) {
		m_preferredAdapter = index;
	}
}

AdapterData* AdapterReader::chooseAdapter() {
	if (s_m_adapters.empty()) {
		AdapterReader::GetAdapters();
	}

	if (m_preferredAdapter != -1) {
		return &s_m_adapters.at(m_preferredAdapter);
	}

	AdapterData* adapter = nullptr;

	if (!s_m_adapters.empty()) {
		auto it = std::max_element(s_m_adapters.begin(), s_m_adapters.end(), [](const AdapterData& a, const AdapterData& b) {
			return a.m_description.DedicatedVideoMemory < b.m_description.DedicatedVideoMemory;
		});
		adapter = &*it;
	}
	return adapter;
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