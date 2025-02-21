#ifdef USE_DIRECTX
#include "DXMesh.hpp"
#include "directx/window/DXDevice.hpp"
#include "directx/util/DXError.hpp"
#include "directx/util/DebugUtil.hpp"

#include <d3d11_1.h>

int Mesh::meshesCount = 0;
int Mesh::drawCalls = 0;

inline size_t calc_vertex_size(const VertexAttribute* attrs) {
	size_t vertexSize = 0;
	for (int i = 0; attrs[i].size; i++) {
		vertexSize += attrs[i].size;
	}
	return vertexSize;
}

Mesh::Mesh(const MeshData& data)
	: Mesh(data.vertices.data(),
		   data.vertices.size() / calc_vertex_size(data.attrs.data()),
		   data.indices.data(),
		   data.indices.size(),
		   data.attrs.data()) {}

Mesh::Mesh(const float* vertexBuffer, size_t vertices, const DWORD* indexBuffer, size_t indices, const VertexAttribute* attrs) :
	m_vertexSize(0),
	m_vertices(vertices),
	m_indices(indices),
	m_p_vertexBuffer(nullptr),
	m_p_indexBuffer(nullptr),
	m_stride(0)
{
	for (int i = 0; attrs[i].size; i++) {
		m_vertexSize += attrs[i].size;
	}

	reload(vertexBuffer, vertices, indexBuffer, indices);

	meshesCount++;
}

Mesh::~Mesh() {
	releaseResources();
	meshesCount--;
}

void Mesh::reload(const float* vertexBuffer, size_t vertices, const DWORD* indexBuffer, size_t indices) {
	if (vertexBuffer == nullptr || vertices == 0) return;

	releaseResources();

	auto device = DXDevice::getDevice();

	m_vertices = vertices;
	m_stride = sizeof(float) * m_vertexSize;

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(float) * vertices * m_vertexSize;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA bufferData;
	ZeroMemory(&bufferData, sizeof(bufferData));
	bufferData.pSysMem = vertexBuffer;

	CHECK_ERROR2(device->CreateBuffer(&bufferDesc, &bufferData, &m_p_vertexBuffer),
		L"Failed to create vertex buffer");

	SET_DEBUG_OBJECT_NAME(m_p_vertexBuffer, "Vertex buffer");

	if (indexBuffer != nullptr && indices > 0) {
		m_indices = indices;

		bufferDesc.ByteWidth = sizeof(DWORD) * indices;
		bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

		bufferData.pSysMem = indexBuffer;

		CHECK_ERROR2(device->CreateBuffer(&bufferDesc, &bufferData, &m_p_indexBuffer),
			L"Failed to create index buffer");

		SET_DEBUG_OBJECT_NAME(m_p_indexBuffer, "Index buffer");
	}
}

void Mesh::draw(D3D_PRIMITIVE_TOPOLOGY primitive) const {
	auto context = DXDevice::getContext();

	UINT offset = 0;

	context->IASetPrimitiveTopology(primitive);
	context->IASetVertexBuffers(0, 1, &m_p_vertexBuffer, &m_stride, &offset);
	if (m_p_indexBuffer == nullptr) {
		context->Draw(m_vertices, 0);
	}
	else {
		context->IASetIndexBuffer(m_p_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
		context->DrawIndexed(m_indices, 0, 0);
	}
    drawCalls++;
}

void Mesh::releaseResources() {
	if (m_p_vertexBuffer != nullptr) {
		m_p_vertexBuffer->Release();
		m_p_vertexBuffer = nullptr;
	}
	if (m_p_indexBuffer != nullptr) {
		m_p_indexBuffer->Release();
		m_p_indexBuffer = nullptr;
	}
}

#endif // USE_DIRECTX