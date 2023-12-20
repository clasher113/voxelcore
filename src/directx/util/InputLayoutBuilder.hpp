#ifndef INPUT_LAYOUT_BUILDER_HPP
#define INPUT_LAYOUT_BUILDER_HPP

#include <vector>
#include <string_view>
#include <d3d11_1.h>

class InputLayoutBuilder {
public:
	static void add(const std::string_view& name, size_t size);
	static void clear();
	static UINT getSize();
	static D3D11_INPUT_ELEMENT_DESC* get();
private:
	static std::vector<D3D11_INPUT_ELEMENT_DESC> inputLayout;
};

#endif // !INPUT_LAYOUT_BUILDER_HPP
