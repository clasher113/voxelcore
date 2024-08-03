#ifndef INPUT_LAYOUT_BUILDER_HPP
#define INPUT_LAYOUT_BUILDER_HPP

#include <d3d11_1.h>

class InputLayoutBuilder {
public:
	static HRESULT create(ID3D11Device* device, ID3D10Blob* shader, ID3D11InputLayout** inputLayout);
};

#endif // !INPUT_LAYOUT_BUILDER_HPP
