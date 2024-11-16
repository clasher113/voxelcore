#ifndef CONSTANT_BUFFER_HPP
#define CONSTANT_BUFFER_HPP

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include "ShaderTypes.hpp"

#include <d3dcompiler.h>
#include <unordered_map>
#include <DirectXMath.h>
#include <glm\glm.hpp>
#include <string_view>

struct ID3D10Blob;
struct ID3D11Buffer;

struct ConstantBufferVariable {
	size_t startOffset;
	size_t size;
};

struct ConstantBufferData {
	std::unordered_map<std::string, ConstantBufferVariable> bufferVars;
	size_t size = 0;
	unsigned int shaderType = 0;
};

class ConstantBuffer {
public:
	ConstantBuffer(const ConstantBufferData& data);
	~ConstantBuffer();

	void uniformMatrix(const std::string_view& name, const DirectX::XMFLOAT4X4& matrix);
	void uniformMatrix(const std::string_view& name, const glm::mat4& matrix);
	void uniform1i(const std::string_view& name, int x);
	void uniform2i(const std::string_view& name, glm::ivec2 xy);
	void uniform1f(const std::string_view& name, float x);
	void uniform2f(const std::string_view& name, float x, float y);
	void uniform2f(const std::string_view& name, const DirectX::XMFLOAT2& xy);
	void uniform2f(const std::string_view& name, const glm::vec2& xy);
	void uniform3f(const std::string_view& name, float x, float y, float z);
	void uniform3f(const std::string_view& name, const DirectX::XMFLOAT3& xyz);
	void uniform3f(const std::string_view& name, const glm::vec3& xyz);

	void bind(UINT startSlot = 0u);
	void applyChanges();

private:
	bool m_hasChanges;
	size_t m_size;
	unsigned char* m_p_data;
	unsigned int m_shaderType;

	ID3D11Buffer* m_p_buffer;
	std::unordered_map<std::string, ConstantBufferVariable> m_bufferVars;

	void modifyVariable(const std::string_view& name, const void* src);
};

#endif // !CONSTANT_BUFFER_HPP