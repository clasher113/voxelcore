#pragma once

#ifdef _DEBUG
#include <d3d11.h>
#define SET_DEBUG_OBJECT_NAME(RESOURCE, NAME) SetDebugObjectName(RESOURCE, NAME);
inline void SetDebugObjectName(_In_ ID3D11DeviceChild* resource, _In_z_ const char* name) {
	if (resource == nullptr) return;
	resource->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(name), name);
}
#else
#define SET_DEBUG_OBJECT_NAME(RESOURCE, NAME)
#endif // DEBUG