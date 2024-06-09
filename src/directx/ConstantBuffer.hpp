#ifndef CONSTANT_BUFFER_HPP
#define CONSTANT_BUFFER_HPP

#include "ShaderTypes.hpp"

#include <d3dcompiler.h>
#include <unordered_map>
#include <DirectXMath.h>
#include <glm\glm.hpp>
#include <string_view>

struct ID3D10Blob;
struct ID3D11Buffer;

struct ConstantBufferVariable {
	UINT startOffset;
	UINT size;
};

class ConstantBuffer {
public:
	ConstantBuffer(UINT size, unsigned int shaderType);
	~ConstantBuffer();

	void addVariable(const std::string& name, const ConstantBufferVariable& variable);

	void uniformMatrix(const std::string_view& name, const DirectX::XMFLOAT4X4& matrix);
	void uniformMatrix(const std::string_view& name, const glm::mat4& matrix);
	void uniform1i(const std::string_view& name, int x);
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
	UINT m_size;
	unsigned char* m_p_data;
	unsigned int m_shaderType;

	ID3D11Buffer* m_p_buffer;
	std::unordered_map<std::string, ConstantBufferVariable> m_bufferVars;

	void modifyVariable(const std::string_view& name, const void* src);
};

#endif // !CONSTANT_BUFFER_HPP