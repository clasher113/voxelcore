#ifndef DX_LINE_HPP
#define DX_LINE_HPP

#include "../ConstantBuffer.hpp"

class Mesh;
struct ID3D11GeometryShader;

struct CB_LineWidth {
	float lineWidth;
};

class DXLine {
public:
	static void init();
	static void setWidth(float width);

	static void draw(Mesh* mesh);
private:
	static float m_width;
	static ID3D11GeometryShader* s_m_p_geometryShader;
	static ConstantBuffer<CB_LineWidth>* s_m_p_cbLineWidth;
};

#endif // !DX_LINE_HPP
