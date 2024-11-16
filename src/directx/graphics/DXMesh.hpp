#ifndef DX_MESH_HPP
#define DX_MESH_HPP

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <d3dcommon.h>
#include "typedefs.hpp"
#include "graphics/core/MeshData.hpp"

struct ID3D11Buffer;

class Mesh {
public:
	Mesh(const MeshData& data);
	Mesh(const float* vertexBuffer, size_t vertices, const DWORD* indexBuffer, size_t indices, const vattr* attrs);
	Mesh(const float* vertexBuffer, size_t vertices, const vattr* attrs) :
		Mesh(vertexBuffer, vertices, nullptr, 0, attrs) {};
	~Mesh();

	void reload(const float* vertexBuffer, size_t vertices, const DWORD* indexBuffer = nullptr, size_t indices = 0);
	void draw(D3D_PRIMITIVE_TOPOLOGY primitive = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

private:
	size_t m_vertices;
	size_t m_indices;
	size_t m_vertexSize;
	UINT m_stride;

	ID3D11Buffer* m_p_vertexBuffer;
	ID3D11Buffer* m_p_indexBuffer;

	void releaseResources();
public:
	static int meshesCount;
    static int drawCalls;
};

#endif // !DX_MESH_HPP
