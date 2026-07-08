#pragma once

#include "typedefs.hpp"
#include "graphics/core/MeshData.hpp"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <d3d11_1.h>

#undef OPTIONAL

struct MeshStats {
	static inline int meshesCount = 0;
	static inline int drawCalls = 0;
};

struct IndexBufferData {
	const uint32_t* indices;
	size_t indicesCount;
};

template <typename VertexStructure>
class Mesh {
	struct IndexBuffer {
		ID3D11Buffer* m_p_indexBuffer;
		size_t m_indexCount;
	};
public:
	explicit Mesh(const MeshData<VertexStructure>& data);
	Mesh(
		const VertexStructure* vertexBuffer,
		size_t vertices,
		std::vector<IndexBufferData> indices
	);
	Mesh(const VertexStructure* vertexBuffer, size_t vertices)
		: Mesh<VertexStructure>(vertexBuffer, vertices, {}) {};
	~Mesh();

	void reload(
		const VertexStructure* vertexBuffer,
		size_t vertexCount,
		const std::vector<IndexBufferData>& indices
	);
	void reload(const VertexStructure* vertexBuffer, size_t vertexCount) {
		static const std::vector<IndexBufferData> indices{};
		reload(vertexBuffer, vertexCount, indices);
	}

	void draw(D3D_PRIMITIVE_TOPOLOGY primitive, int iboIndex = 0) const;
	void draw() const;
private:
	size_t m_vertexCount;
	UINT m_stride;

	ID3D11Buffer* m_p_vertexBuffer;
	std::vector<IndexBuffer> m_indexBuffers;

	void releaseResources();
};

#include "Mesh.inl"
