#ifdef USE_DIRECTX
#ifndef CONSTANT_BUFFER_HPP
#define CONSTANT_BUFFER_HPP

#include "window/DXDevice.hpp"
#include "util/DXError.hpp"
#include "../util/stringutil.h"

#include <d3d11_1.h>

template<class Type>
class ConstantBuffer {
private:
	ConstantBuffer(const ConstantBuffer<Type>& buffer);
public:
	ConstantBuffer() {
		auto device = DXDevice::getDevice();

		D3D11_BUFFER_DESC desc{};
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;
		desc.ByteWidth = static_cast<UINT>(sizeof(Type) + (16 - sizeof(Type) % 16));
		desc.StructureByteStride = 0;

		DXError::checkError(device->CreateBuffer(&desc, 0, &buffer),
			L"Failed to create constant buffer: " /*+ str2wstr_utf8(typeid(Type).name())*/); // TODO print type name
	};

	~ConstantBuffer() {
		buffer->Release();
	};

	bool applyChanges() {
		auto context = DXDevice::getContext();

		D3D11_MAPPED_SUBRESOURCE resource{};
		if (!DXError::checkError(context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource),
			L"Failed to map constant buffer")) return false;
		CopyMemory(resource.pData, &data, sizeof(Type));
		context->Unmap(buffer, 0);

		return true;
	};

	void bind() {
		auto context = DXDevice::getContext();

		context->VSSetConstantBuffers(0, 1, &buffer);
	};

	Type data;
private:
	ID3D11Buffer* buffer;
};

#endif // !CONSTANT_BUFFER_HPP
#endif // USE_DIRECTX