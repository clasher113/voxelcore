#pragma once

#include "ShaderTypes.hpp"

#include <unordered_map>
#include <glm\fwd.hpp>
#include <string>

struct ID3D11Buffer;

struct ConstantBufferVariable {
	size_t startOffset;
	size_t size;
};

struct ConstantBufferData {
	struct Data {
		std::unordered_map<std::string, ConstantBufferVariable> bufferVars;
		unsigned int bindSlot = 0;
		ShaderType shaderType = static_cast<ShaderType>(0);
	};
	std::unordered_map<std::string, Data> data;
};

class ConstantBuffer {
public:
	ConstantBuffer(const ConstantBufferData& data);
	~ConstantBuffer();

	void uniformMatrix(const std::string_view& name, const glm::mat3& matrix);
	void uniformMatrix(const std::string_view& name, const glm::mat4& matrix);
	void uniform1i(const std::string_view& name, int x);
	void uniform2i(const std::string_view& name, const glm::ivec2& xy);
	void uniform1f(const std::string_view& name, float x);
	void uniform2f(const std::string_view& name, float x, float y);
	void uniform2f(const std::string_view& name, const glm::vec2& xy);
	void uniform3f(const std::string_view& name, float x, float y, float z);
	void uniform3f(const std::string_view& name, const glm::vec3& xyz);
	void uniform4f(const std::string_view& name, const glm::vec4& xyzw);

	void uniform1v(const std::string_view& name, int length, const int* v);
	void uniform1v(const std::string_view& name, int length, const float* v);
	void uniform2v(const std::string_view& name, int length, const float* v);
	void uniform3v(const std::string_view& name, int length, const float* v);
	void uniform4v(const std::string_view& name, int length, const float* v);

	void bind();
	void applyChanges();

private:
	struct Buffer {
		ID3D11Buffer* m_p_buffer = nullptr;
		ShaderType m_shaderType = static_cast<ShaderType>(0);
		unsigned int m_bindSlot = 0;
		size_t m_memoryOffset = 0;
		size_t m_sizeBytes = 0;
		bool m_hasChanges = false;
	};

	bool m_hasChanges;
	unsigned char* m_p_data;

	Buffer* m_p_buffers;
	size_t m_buffersNum;
	std::unordered_map<std::string, std::pair<Buffer*, ConstantBufferVariable>> m_bufferVars;

	void modifyVariable(const std::string_view& name, const void* src);
	void modifyArray(const std::string_view& name, size_t size, size_t rowPitch, const void* src);
};