#ifdef USE_DIRECTX
#include "DXMesh.hpp"
#include "../window/DXDevice.hpp"
#include "../util/DXError.hpp"

#include <d3d11_1.h>

int Mesh::meshesCount = 0;

Mesh::Mesh(const float* vertexBuffer, size_t vertices, const DWORD* indexBuffer, size_t indices, const vattr* attrs) :
    m_vertexSize(0),
    m_vertices(vertices),
    m_indices(indices),
    m_p_vertexBuffer(nullptr),
    m_p_indexBuffer(nullptr)
{
    for (int i = 0; attrs[i].size; i++) {
        m_vertexSize += attrs[i].size;
    }

    reload(vertexBuffer, vertices, indexBuffer, indices);

    meshesCount++;
}

Mesh::~Mesh() {
    if (m_p_vertexBuffer != nullptr) m_p_vertexBuffer->Release();
    if (m_p_indexBuffer != nullptr) m_p_indexBuffer->Release();
    meshesCount--;
}

void Mesh::reload(const float* vertexBuffer, size_t vertices, const DWORD* indexBuffer, size_t indices) {
    if (vertexBuffer == nullptr || vertices == 0) return;
    if (m_p_vertexBuffer != nullptr) m_p_vertexBuffer->Release();
    if (m_p_indexBuffer != nullptr) m_p_indexBuffer->Release();

    auto device = DXDevice::getDevice();

    m_vertices = vertices;

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

    if (indexBuffer != nullptr && indices > 0) {
        m_indices = indices;

        bufferDesc.ByteWidth = sizeof(DWORD) * vertices * m_vertexSize;
        bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        bufferData.pSysMem = indexBuffer;

        CHECK_ERROR2(device->CreateBuffer(&bufferDesc, &bufferData, &m_p_indexBuffer),
            L"Failed to create index buffer");
    }
}

void Mesh::draw(D3D_PRIMITIVE_TOPOLOGY primitive) {
    if (m_p_vertexBuffer == nullptr) return;
    UINT stride = sizeof(float) * m_vertexSize;
    UINT offset = 0;
    auto context = DXDevice::getContext();
    context->IASetVertexBuffers(0, 1, &m_p_vertexBuffer, &stride, &offset);
    context->IASetPrimitiveTopology(primitive);
    if (m_p_indexBuffer == nullptr) {
        context->Draw(m_vertices, 0);
    }
    else {
        context->IASetIndexBuffer(m_p_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
        context->DrawIndexed(m_indices, 0, 0);
    }
}

#endif // USE_DIRECTX