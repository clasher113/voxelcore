#ifndef DEBUG_UTIL_HPP
#define DEBUG_UTIL_HPP
#ifdef _DEBUG

inline void SetDebugObjectName(_In_ ID3D11DeviceChild* resource, _In_z_ const char* name) {
	resource->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(name), name);
}

#endif // DEBUG
#endif // !DEBUG_UTIL_HPP
