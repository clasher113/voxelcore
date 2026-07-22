#pragma once

#include <vector> 
#include <string>
#include <d3d11_1.h>
#include <wrl/client.h>

class AdapterData {
public:
	AdapterData(Microsoft::WRL::ComPtr<IDXGIAdapter> pAdapter);
	Microsoft::WRL::ComPtr<IDXGIAdapter> m_adapter;
	DXGI_ADAPTER_DESC m_description;
	std::string m_vendor;
};

class AdapterReader {
public:
	static const std::vector<AdapterData>& GetAdapters();
	static void setPreferedAdapter(UINT index);
	static AdapterData* chooseAdapter();
private:
	static inline std::vector<AdapterData> s_m_adapters;
	static inline UINT m_preferredAdapter = -1;
};