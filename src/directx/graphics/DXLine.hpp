#ifndef DX_LINE_HPP
#define DX_LINE_HPP

class Mesh;
class ConstantBuffer;
struct ID3D11GeometryShader;

class DXLine {
public:
	static void initialize();
	static void terminate();
	static void setWidth(float width);

	static void draw(Mesh* mesh);
private:
	static float m_width;
	static ID3D11GeometryShader* s_m_p_geometryShader;
	static ConstantBuffer* s_m_p_cbLineWidth;
};

#endif // !DX_LINE_HPP
