#ifndef DX_MESH_HPP
#define DX_MESH_HPP

#define NOMINMAX

#include <d3dcommon.h>
#include "../../typedefs.hpp"

struct ID3D11Buffer;

struct vattr {
	ubyte size;
};

class Mesh {
public:
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
public:
	static int meshesCount;
};

#endif // !DX_MESH_HPP
