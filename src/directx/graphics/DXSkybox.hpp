#ifndef DX_SKYBOX_HPP
#define DX_SKYBOX_HPP

#include "typedefs.hpp"
#include "maths/fastmaths.hpp"

#include <string>
#include <vector>
#include <glm/glm.hpp>

class Mesh;
class Assets;
class Batch3D;
class Shader;
class DrawContext;
class Camera;
struct ID3D11Texture2D;
struct ID3D11RenderTargetView;
struct ID3D11ShaderResourceView;

struct skysprite {
	std::string texture;
	float phase;
	float distance;
	bool emissive;
};

class Skybox {
public:
	Skybox(uint size, Shader* shader);
	~Skybox();

	void draw(
		const DrawContext& pctx,
		const Camera& camera,
		const Assets& assets,
		float daytime,
		float fog
	);

	void refresh(const DrawContext& pctx, float t, float mie, uint quality);

	void bind() const;
	void unbind() const;
	bool isReady() const {
		return ready;
	}
	const glm::vec3 getLightDir() const {
		return lightDir;
	}
private:
	bool ready = false;
	uint m_size;
	int frameid = 0;
	float prevMie = -1.0f;
	float prevT = -1.0f;

	glm::vec3 lightDir;

	Shader* m_p_shader;
	Mesh* m_p_mesh;
	Batch3D* m_p_batch3d;

	FastRandom random;

	ID3D11Texture2D* m_p_texture;
	ID3D11RenderTargetView* m_p_renderTargets[6];
	ID3D11ShaderResourceView* m_p_resourceView;

	std::vector<skysprite> sprites;

	void drawStars(float angle, float opacity);
	void drawBackground(const Camera& camera, const Assets& assets, int width, int height);
	void refreshFace(uint face);
};

#endif // !DX_SKYBOX_HPP
