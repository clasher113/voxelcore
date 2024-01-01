#ifndef INPUT_LAYOUT_BUILDER_HPP
#define INPUT_LAYOUT_BUILDER_HPP

#include <vector>
#include <d3d11_1.h>

class InputLayoutBuilder {
public:
	static void build(ID3D10Blob* shader);
	static void clear();
	static UINT getSize();
	static D3D11_INPUT_ELEMENT_DESC* getData();
private:
	static std::vector<D3D11_INPUT_ELEMENT_DESC> inputLayout;
};

#endif // !INPUT_LAYOUT_BUILDER_HPP
