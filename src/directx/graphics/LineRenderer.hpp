#pragma once

template<typename VertexStructure> class Mesh;
class ConstantBuffer;
struct ID3D11GeometryShader;
struct ID3D11Device;

class LineRenderer {
private:
	friend class Device;
	static void initialize(ID3D11Device* device);
	static void terminate();
public:
	static void setWidth(float width);

	template<typename VertexStructure>
	static void draw(const Mesh<VertexStructure>& mesh);
private:
	static inline bool s_m_initialized = false;
	static inline float s_m_width = 1.f;
	static inline ID3D11GeometryShader* s_m_p_geometryShader = nullptr;
	static inline ConstantBuffer* s_m_p_cbLineWidth = nullptr;
};