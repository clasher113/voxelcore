#ifdef USE_DIRECTX
#include "InputLayoutBuilder.hpp"

std::vector<D3D11_INPUT_ELEMENT_DESC> InputLayoutBuilder::inputLayout;

void InputLayoutBuilder::add(const std::string_view& name, size_t size) {
	D3D11_INPUT_ELEMENT_DESC desc{};
	desc.SemanticName = name.data();
	desc.SemanticIndex = 0u;
	switch (size) {
	case 1: desc.Format = DXGI_FORMAT_R32_FLOAT; break;
	case 2: desc.Format = DXGI_FORMAT_R32G32_FLOAT; break;
	case 3: desc.Format = DXGI_FORMAT_R32G32B32_FLOAT; break;
	case 4: desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
	}
	desc.InputSlot = 0;
	desc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	desc.InstanceDataStepRate = 0;
	inputLayout.emplace_back(desc);
}

void InputLayoutBuilder::clear() {
	inputLayout.clear();
}

UINT InputLayoutBuilder::getSize() {
	return static_cast<UINT>(inputLayout.size());
}

D3D11_INPUT_ELEMENT_DESC* InputLayoutBuilder::get() {
	return inputLayout.data();
}

#endif // USE_DIRECTX