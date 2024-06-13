#ifndef DEBUG_UTIL_HPP
#define DEBUG_UTIL_HPP
#ifdef _DEBUG
#define SET_DEBUG_OBJECT_NAME(RESOURCE, NAME) SetDebugObjectName(RESOURCE, NAME);
#else
#define SET_DEBUG_OBJECT_NAME(RESOURCE, NAME)
#endif // DEBUG

inline void SetDebugObjectName(_In_ ID3D11DeviceChild* resource, _In_z_ const char* name) {
	if (resource == nullptr) return;
	resource->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(name), name);
}

#endif // !DEBUG_UTIL_HPP
