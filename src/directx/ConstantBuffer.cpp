#ifdef USE_DIRECTX
#include "ConstantBuffer.hpp"

#include "window/Device.hpp"
#include "util/Error.hpp"
#include "util\DebugUtil.hpp"
#include "debug/Logger.hpp"

#include <glm\gtc\type_ptr.hpp>
#ifdef _DEBUG
#include <iostream>
#endif // _DEBUG

const size_t ARRAY_ROW_PITCH = 16;

static debug::Logger logger("cbuffer");

ConstantBuffer::ConstantBuffer(const ConstantBufferData& data) :
	m_hasChanges(false),
	m_buffersNum(data.data.size())
{
	ID3D11Device* const device = Device::getDevice();

	m_p_buffers = new Buffer[m_buffersNum];
	size_t cpuBufferSize = 0;
	size_t index = 0;
	std::string logStr;

	for (const auto& [ name, bufferData ] : data.data) {
		Buffer& buffer = m_p_buffers[index];
		size_t bufferSizeBytes = 0;

		for (const auto& [name, cbuffVar] : bufferData.bufferVars) {
			bufferSizeBytes += cbuffVar.size;

			ConstantBufferVariable varCopy {
				cpuBufferSize + cbuffVar.startOffset,
				cbuffVar.size
			};

			m_bufferVars.emplace(name, std::pair<Buffer*, ConstantBufferVariable>{ &buffer, varCopy });
		}

		buffer.m_bindSlot = bufferData.bindSlot;
		buffer.m_memoryOffset = cpuBufferSize;
		buffer.m_shaderType = bufferData.shaderType;
		buffer.m_sizeBytes = bufferSizeBytes + (16 - bufferSizeBytes % 16);

		D3D11_BUFFER_DESC desc{
			/* UINT ByteWidth */			buffer.m_sizeBytes,
			/* D3D11_USAGE Usage */			D3D11_USAGE_DYNAMIC,
			/* UINT BindFlags */			D3D11_BIND_CONSTANT_BUFFER,
			/* UINT CPUAccessFlags */		D3D11_CPU_ACCESS_WRITE,
			/* UINT MiscFlags */			0,
			/* UINT StructureByteStride */	0,
		};

		CHECK_ERROR2(device->CreateBuffer(&desc, 0, &buffer.m_p_buffer),
			L"Failed to create constant buffer");

		SET_DEBUG_OBJECT_NAME(buffer.m_p_buffer, "Constant Buffer");

		logStr += std::to_string(index) + ' ' + name + ": vars " + std::to_string(bufferData.bufferVars.size()) + " size " +
			std::to_string(buffer.m_sizeBytes);
		cpuBufferSize += buffer.m_sizeBytes;
		index++;
		if (index < data.data.size()) {
			logStr += ", ";
		}
	}

	logger.info() << "cbuffer info: " << index << " buffers with overral size " << cpuBufferSize;
	if (!logStr.empty()) {
		logger.info() << logStr;
	}

	m_p_data = new unsigned char[cpuBufferSize];
	memset(m_p_data, 0, cpuBufferSize);
}

ConstantBuffer::~ConstantBuffer() {
	for (size_t i = 0; i < m_buffersNum; i++) {
		m_p_buffers[i].m_p_buffer->Release();
	}
	delete[] m_p_buffers;
	delete[] m_p_data;
}

void ConstantBuffer::uniformMatrix(const std::string_view& name, const glm::mat3& matrix) {
	modifyVariable(name, glm::value_ptr(matrix));
}

void ConstantBuffer::uniformMatrix(const std::string_view& name, const glm::mat4& matrix) {
	modifyVariable(name, glm::value_ptr(matrix));
}

void ConstantBuffer::uniform1i(const std::string_view& name, int x) {
	modifyVariable(name, &x);
}

void ConstantBuffer::uniform2i(const std::string_view& name, const glm::ivec2& xy) {
	modifyVariable(name, glm::value_ptr(xy));
}

void ConstantBuffer::uniform1f(const std::string_view& name, float x) {
	modifyVariable(name, &x);
}

void ConstantBuffer::uniform2f(const std::string_view& name, float x, float y) {
	uniform2f(name, glm::vec2(x, y));
}

void ConstantBuffer::uniform2f(const std::string_view& name, const glm::vec2& xy) {
	modifyVariable(name, glm::value_ptr(xy));
}

void ConstantBuffer::uniform3f(const std::string_view& name, float x, float y, float z) {
	uniform3f(name, glm::vec3(x, y, z));
}

void ConstantBuffer::uniform3f(const std::string_view& name, const glm::vec3& xyz) {
	modifyVariable(name, glm::value_ptr(xyz));
}

void ConstantBuffer::uniform4f(const std::string_view& name, const glm::vec4& xyzw) {
	modifyVariable(name, glm::value_ptr(xyzw));
}

void ConstantBuffer::uniform1v(const std::string_view& name, int length, const int* v) {
	modifyArray(name, length * sizeof(int), sizeof(int), v);
}

void ConstantBuffer::uniform1v(const std::string_view& name, int length, const float* v) {
	modifyArray(name, length * sizeof(float), sizeof(float), v);
}

void ConstantBuffer::uniform2v(const std::string_view& name, int length, const float* v) {
	modifyArray(name, length * sizeof(glm::vec2), sizeof(glm::vec2), v);
}

void ConstantBuffer::uniform3v(const std::string_view& name, int length, const float* v) {
	modifyArray(name, length * sizeof(glm::vec3), sizeof(glm::vec3), v);
}

void ConstantBuffer::uniform4v(const std::string_view& name, int length, const float* v) {
	modifyArray(name, length * sizeof(glm::vec4), sizeof(glm::vec4), v);
}

void ConstantBuffer::modifyVariable(const std::string_view& name, const void* src) {
	auto item = m_bufferVars.find(name.data());
	if (item == m_bufferVars.end()) {
#ifdef _DEBUG
		std::cout << __FUNCTION__ << "(); Unknown variable name: " << name << std::endl;
#endif // _DEBUG
		return;
	}
	const auto& [buffer, variable] = item->second;
	assert(variable.startOffset - buffer->m_memoryOffset + variable.size <= buffer->m_sizeBytes);
	memcpy(&m_p_data[variable.startOffset], src, variable.size);
	buffer->m_hasChanges = m_hasChanges = true;
}

void ConstantBuffer::modifyArray(const std::string_view& name, size_t size, size_t rowPitch, const void* src) {
	auto array = m_bufferVars.find(name.data());
	if (array == m_bufferVars.end()) {
#ifdef _DEBUG
		std::cout << __FUNCTION__ << "(); Unknown array name: " << name << std::endl;
#endif // _DEBUG
		return;
	}
	const auto& [buffer, variable] = array->second;
	if (rowPitch == ARRAY_ROW_PITCH) {
		assert(variable.startOffset - buffer->m_memoryOffset + size <= buffer->m_sizeBytes);
		memcpy(&m_p_data[variable.startOffset], src, size);
	} else {
		const size_t rows = size / rowPitch;
		assert(variable.startOffset - buffer->m_memoryOffset + rows * ARRAY_ROW_PITCH <= buffer->m_sizeBytes);

		for (size_t i = 0; i < rows; i++) {
			memcpy(&m_p_data[variable.startOffset + i * ARRAY_ROW_PITCH], &static_cast<const uint8_t*>(src)[i * rowPitch], rowPitch);
		}
	}
	buffer->m_hasChanges = m_hasChanges = true;
}

void ConstantBuffer::applyChanges() {
	ID3D11DeviceContext* const context = Device::getContext();

	for (size_t i = 0; i < m_buffersNum; i++) {
		Buffer& buffer = m_p_buffers[i];
		if (!buffer.m_hasChanges) continue;

		D3D11_MAPPED_SUBRESOURCE resource{};

		CHECK_ERROR2(context->Map(buffer.m_p_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource),
			L"Failed to map constant buffer");
		CopyMemory(resource.pData, m_p_data + buffer.m_memoryOffset, buffer.m_sizeBytes);
		context->Unmap(buffer.m_p_buffer, 0);
		
		buffer.m_hasChanges = false;
	}

	m_hasChanges = false;
}

void ConstantBuffer::bind() {
	ID3D11DeviceContext* const context = Device::getContext();

	if (m_hasChanges) applyChanges();
	for (size_t i = 0; i < m_buffersNum; i++) {
		Buffer& buffer = m_p_buffers[i];

		if (buffer.m_shaderType & VERTEX)		context->VSSetConstantBuffers(buffer.m_bindSlot, 1, &buffer.m_p_buffer);
		if (buffer.m_shaderType & PIXEL)		context->PSSetConstantBuffers(buffer.m_bindSlot, 1, &buffer.m_p_buffer);
		if (buffer.m_shaderType & GEOMETRY)		context->GSSetConstantBuffers(buffer.m_bindSlot, 1, &buffer.m_p_buffer);
	}
}

#endif // USE_DIRECTX