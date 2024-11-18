#ifdef USE_DIRECTX
#include "ConstantBuffer.hpp"
#include "window/DXDevice.hpp"
#include "util/DXError.hpp"
#include "util\DebugUtil.hpp"
#include "math\DXMathHelper.hpp"

#include <glm\gtc\type_ptr.hpp>
#include <iostream>

ConstantBuffer::ConstantBuffer(const ConstantBufferData& data) :
	m_bufferVars(data.bufferVars),
	m_size(data.size),
	m_shaderType(data.shaderType),
	m_hasChanges(false),
	m_p_data(new unsigned char[data.size]),
	m_p_buffer(nullptr)
{
	auto device = DXDevice::getDevice();

	D3D11_BUFFER_DESC desc{};
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	desc.ByteWidth = static_cast<UINT>(m_size + (16 - m_size % 16));
	desc.StructureByteStride = 0;

	CHECK_ERROR2(device->CreateBuffer(&desc, 0, &m_p_buffer),
		L"Failed to create constant buffer");

	SET_DEBUG_OBJECT_NAME(m_p_buffer, "Constant Buffer")

	memset(m_p_data, 0, m_size);
}

ConstantBuffer::~ConstantBuffer() {
	m_p_buffer->Release();
	delete[] m_p_data;
}

void ConstantBuffer::uniformMatrix(const std::string_view& name, const DirectX::XMFLOAT4X4& matrix) {
	DirectX::XMFLOAT4X4 mat(transpose(matrix));
	modifyVariable(name, &mat._11);
}

void ConstantBuffer::uniformMatrix(const std::string_view& name, const glm::mat4& matrix) {
	modifyVariable(name, glm::value_ptr(glm::transpose(matrix)));
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
	uniform2f(name, DirectX::XMFLOAT2(x, y));
}

void ConstantBuffer::uniform2f(const std::string_view& name, const DirectX::XMFLOAT2& xy) {
	modifyVariable(name, &xy.x);
}

void ConstantBuffer::uniform2f(const std::string_view& name, const glm::vec2& xy) {
	modifyVariable(name, glm::value_ptr(xy));
}

void ConstantBuffer::uniform3f(const std::string_view& name, float x, float y, float z) {
	uniform3f(name, DirectX::XMFLOAT3(x, y, z));
}

void ConstantBuffer::uniform3f(const std::string_view& name, const DirectX::XMFLOAT3& xyz) {
	modifyVariable(name, &xyz.x);
}

void ConstantBuffer::uniform3f(const std::string_view& name, const glm::vec3& xyz) {
	modifyVariable(name, glm::value_ptr(xyz));
}

void ConstantBuffer::modifyVariable(const std::string_view& name, const void* src) {
	auto item = m_bufferVars.find(name.data());
	if (item == m_bufferVars.end()) {
#ifdef _DEBUG
		std::cout << __FUNCTION__ << "(); Unknown variable name: " << name << std::endl;
#endif // _DEBUG
		return;
	}
	ConstantBufferVariable& var = item->second;
	memcpy(&m_p_data[var.startOffset], src, var.size);
	m_hasChanges = true;
}

void ConstantBuffer::applyChanges() {
	auto context = DXDevice::getContext();

	D3D11_MAPPED_SUBRESOURCE resource{};
	CHECK_ERROR2(context->Map(m_p_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource),
		L"Failed to map constant buffer");
	CopyMemory(resource.pData, m_p_data, m_size);
	context->Unmap(m_p_buffer, 0);
	m_hasChanges = false;
}

void ConstantBuffer::bind(UINT startSlot) {
	auto context = DXDevice::getContext();

	if (m_hasChanges) applyChanges();
	if (m_shaderType & VERTEX)		context->VSSetConstantBuffers(startSlot, 1, &m_p_buffer);
	if (m_shaderType & PIXEL)		context->PSSetConstantBuffers(startSlot, 1, &m_p_buffer);
	if (m_shaderType & GEOMETRY)	context->GSSetConstantBuffers(startSlot, 1, &m_p_buffer);
}

#endif // USE_DIRECTX