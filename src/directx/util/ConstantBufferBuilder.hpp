#ifndef CONSTANT_BUFFER_BUILDER_HPP
#define CONSTANT_BUFFER_BUILDER_HPP

#include "../ConstantBuffer.hpp"

class ConstantBufferBuilder {
public:
	void build(ID3D10Blob* shader, unsigned int shaderType);

	ConstantBufferData getData();
private:
	ConstantBufferData m_data;
};

#endif // CONSTANT_BUFFER_BUILDER_HPP