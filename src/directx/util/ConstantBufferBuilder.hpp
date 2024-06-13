#ifndef CONSTANT_BUFFER_BUILDER_HPP
#define CONSTANT_BUFFER_BUILDER_HPP

#include "../ConstantBuffer.hpp"

#include <unordered_map>

class ConstantBufferBuilder {
public:
	void build(ID3D10Blob* shader, unsigned int shaderType);

	ConstantBufferData getData();
private:
	size_t m_size = 0;
	unsigned int m_shaderType;
	std::unordered_map<std::string, ConstantBufferVariable> m_bufferVars;
};

#endif // CONSTANT_BUFFER_BUILDER_HPP