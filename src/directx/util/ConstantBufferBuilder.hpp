#ifndef CONSTANT_BUFFER_BUILDER_HPP
#define CONSTANT_BUFFER_BUILDER_HPP

#include "../ConstantBuffer.hpp"

#include <unordered_map>

class ConstantBufferBuilder {
public:
	static void build(ID3D10Blob* shader, unsigned int shaderType);
	static void clear();
	static ConstantBuffer* create();

private:
	static inline size_t s_m_size = 0;
	static inline unsigned int s_m_shaderType;
	static inline std::unordered_map<std::string, ConstantBufferVariable> s_m_bufferVars;
};

#endif // CONSTANT_BUFFER_BUILDER_HPP