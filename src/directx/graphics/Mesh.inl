#pragma once 

#include "graphics/core/MeshData.hpp"
#include "directx/window/Device.hpp"
#include "directx/util/Error.hpp"
#include "directx/util/DebugUtil.hpp"

inline constexpr size_t calc_size(const VertexAttribute attrs[]) {
    size_t vertexSize = 0;
    for (int i = 0; attrs[i].count; i++) {
        vertexSize += attrs[i].size();
    }
    return vertexSize;
}

template <typename VertexStructure>
inline std::vector<IndexBufferData> convert_to_ibd(const MeshData<VertexStructure>& data) {
    std::vector<IndexBufferData> indices;
    for (const auto& buffer : data.indices) {
        indices.push_back(IndexBufferData{ buffer.data(), buffer.size() });
    }
    return indices;
}

template <typename VertexStructure>
Mesh<VertexStructure>::Mesh(const MeshData<VertexStructure>& data)
    : Mesh(data.vertices.data(),
          data.vertices.size(),
          convert_to_ibd<VertexStructure>(data)
    ) {}

template <typename VertexStructure>
Mesh<VertexStructure>::Mesh(const VertexStructure* vertexBuffer, size_t vertices, std::vector<IndexBufferData> indices) :
    m_vertexCount(0),
    m_stride(0),
    m_p_vertexBuffer(nullptr)
{
    static_assert(
        calc_size(VertexStructure::ATTRIBUTES) == sizeof(VertexStructure)
    );

    const auto& attrs = VertexStructure::ATTRIBUTES;
    MeshStats::meshesCount++;

    reload(vertexBuffer, vertices, std::move(indices));
}

template <typename VertexStructure>
Mesh<VertexStructure>::~Mesh() {
    MeshStats::meshesCount--;
    releaseResources();
}

template <typename VertexStructure>
void Mesh<VertexStructure>::reload(const VertexStructure* vertexBuffer, size_t vertexCount, const std::vector<IndexBufferData>& indices) {
    m_vertexCount = vertexCount;
    if (vertexBuffer == nullptr || vertexCount == 0) return;
    m_stride = calc_size(VertexStructure::ATTRIBUTES);

    releaseResources();

    ID3D11Device* const device = Device::getDevice();


    D3D11_BUFFER_DESC bufferDesc{
        /* UINT ByteWidth */			vertexCount * sizeof(VertexStructure),
        /* D3D11_USAGE Usage */			D3D11_USAGE_DEFAULT,
        /* UINT BindFlags */			D3D11_BIND_VERTEX_BUFFER,
        /* UINT CPUAccessFlags */		0U,
        /* UINT MiscFlags */			0U,
        /* UINT StructureByteStride */	0U
    };

    D3D11_SUBRESOURCE_DATA bufferData{
        /* const void* pSysMem */		vertexBuffer,
        /* UINT SysMemPitch */			0U,
        /* UINT SysMemSlicePitch */		0U,
    };

    CHECK_ERROR2(device->CreateBuffer(&bufferDesc, &bufferData, &m_p_vertexBuffer),
        L"Failed to create vertex buffer");

    SET_DEBUG_OBJECT_NAME(m_p_vertexBuffer, "Vertex buffer");

    if (indices.empty()) return;

    bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    for (const IndexBufferData& indexBufferData : indices) {
        IndexBuffer& indexBuffer = m_indexBuffers.emplace_back(IndexBuffer{ nullptr, 0 });
        indexBuffer.m_indexCount = indexBufferData.indicesCount;

        bufferDesc.ByteWidth = sizeof(uint32_t) * indexBufferData.indicesCount;
        bufferData.pSysMem = indexBufferData.indices;

        CHECK_ERROR2(device->CreateBuffer(&bufferDesc, &bufferData, &indexBuffer.m_p_indexBuffer),
            L"Failed to create index buffer");

        SET_DEBUG_OBJECT_NAME(indexBuffer.m_p_indexBuffer, "Index buffer");
    }
}

template <typename VertexStructure>
void Mesh<VertexStructure>::draw(D3D_PRIMITIVE_TOPOLOGY primitive, int iboIndex) const {
    if (m_vertexCount == 0) return;
    MeshStats::drawCalls++;

    ID3D11DeviceContext* const context = Device::getContext();

    UINT offset = 0;

    context->IASetPrimitiveTopology(primitive);
    context->IASetVertexBuffers(0, 1, &m_p_vertexBuffer, &m_stride, &offset);
    if (m_indexBuffers.empty()) {
        context->Draw(m_vertexCount, 0);
    }
    else if (iboIndex < m_indexBuffers.size()) {
        const IndexBuffer& indexBuffer = m_indexBuffers.at(iboIndex);
        context->IASetIndexBuffer(indexBuffer.m_p_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
        context->DrawIndexed(indexBuffer.m_indexCount, 0, 0);
    }
}

template <typename VertexStructure>
void Mesh<VertexStructure>::draw() const {
    draw(D3D_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

template <typename VertexStructure>
void Mesh<VertexStructure>::releaseResources() {
    if (m_p_vertexBuffer != nullptr) {
        m_p_vertexBuffer->Release();
        m_p_vertexBuffer = nullptr;
    }
    for (IndexBuffer& buffer : m_indexBuffers) {
        buffer.m_p_indexBuffer->Release();
    }
    m_indexBuffers.clear();
}