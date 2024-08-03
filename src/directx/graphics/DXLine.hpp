#ifndef DX_LINE_HPP
#define DX_LINE_HPP

class Mesh;
class ConstantBuffer;
struct ID3D11GeometryShader;
struct ID3D11Device;

class DXLine {
private:
	friend class DXDevice;
	static void initialize(ID3D11Device* device);
	static void terminate();
public:
	static void setWidth(float width);

	static void draw(Mesh* mesh);
private:
	static inline bool s_m_initialized = false;
	static inline float s_m_width = 1.f;
	static inline ID3D11GeometryShader* s_m_p_geometryShader = nullptr;
	static inline ConstantBuffer* s_m_p_cbLineWidth = nullptr;
};

#endif // !DX_LINE_HPP
