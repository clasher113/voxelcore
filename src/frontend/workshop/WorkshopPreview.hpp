#pragma once

#include "graphics/core/Batch2D.hpp"
#include "graphics/core/Batch3D.hpp"
#include "graphics/core/Framebuffer.hpp"
#include "graphics/core/LineBatch.hpp"
#include "graphics/core/Mesh.hpp"
#include "graphics/render/BlocksRenderer.hpp"
#include "graphics/render/ParticlesRenderer.hpp"
#include "items/Inventory.hpp"
#include "maths/aabb.hpp"
#include "window/Camera.hpp"
#include "graphics/render/ModelBatch.hpp"
#include "voxels/Chunks.hpp"

#include <glm/fwd.hpp>

class Engine;
class ContentGfxCache;
class Chunk;
class Shader;
class DrawContext;
namespace gui {
	class UINode;
}
namespace xml {
	class Document;
}
namespace rigging {
	class SkeletonConfig;
}
namespace model {
	struct Model;
}

namespace workshop {

	enum class PrimitiveType : unsigned int;

	class Preview {
	public:
		Preview(Engine& engine, ContentGfxCache& cache);
		~Preview();

		void update(float delta, float sensitivity);
		void updateMesh();
		void updateCache();
		void resetView();

		bool rayCast(float cursorX, float cursorY, size_t& returnIndex);
		void rotate(float x, float y);
		void scale(float value);

		void drawBlock();
		void drawUI();
		void drawSkeleton();
		void drawModel();

		void setBlock(Block* block);
		void setSkeleton(const rigging::SkeletonConfig* skeleton);
		void setModel(model::Model* model);
		void setCurrentAABB(const glm::vec3& a, const glm::vec3& b, PrimitiveType type);
		void setCurrentTetragon(const glm::vec3* const tetragon);
		void setCurrentPrimitive(PrimitiveType type) { primitiveType = type; };
		void updateParticles();

		void setUiDocument(const std::shared_ptr<xml::Document> document, std::shared_ptr<int> enviroment, bool forseUpdate = false);

		void setResolution(uint width, uint height);
		void setBlockSize(const glm::i8vec3& size);

		Texture* getTexture();

		bool lockedKeyboardInput = false;
		bool lmb = false, rmb = false;
		bool drawGrid = true;
		bool drawBlockSize = false;
		bool drawCurrentAABB = false;
		bool drawCurrentTetragon = false;
		bool drawBlockHitbox = false;
		bool drawDirection = true;
		float particlesSpeed = 1.f;
		PrimitiveType lookAtPrimitive;

	private:
		uint windowWidth = 0, windowHeight = 0;
		float refillTimer = 0.f;
		float viewDistance = 2.f;
		glm::i8vec3 blockSize{ 1, 1, 1 };
		glm::vec2 previewRotation{ 225.f, 45.f };
		std::array<glm::vec3, 4> currentTetragon{}, lookAtTetragon{};
		glm::vec3 cameraOffset{0.f, 0.f, 0.f}, cameraPosition{ 0.f, 0.f, 0.f };
		PrimitiveType primitiveType;

		Engine& engine;
		ContentGfxCache& cache;
		Assets& assets;

		BlocksRenderer blockRenderer;
		ModelBatch modelBatch;
		std::shared_ptr<Chunk> chunk;
		Chunks chunks;
		ParticlesRenderer particlesRenderer;
		Camera camera;
		Framebuffer framebuffer;

		std::shared_ptr<Inventory> inventory;
		std::shared_ptr<gui::UINode> currentUI;
		std::shared_ptr<xml::Document> currentDocument;
		Block* currentBlock = nullptr;
		const rigging::SkeletonConfig* currentSkeleton = nullptr;
		model::Model* currentModel = nullptr;

		LineBatch lineBatch;
		Batch2D batch2d;
		Batch3D batch3d;
		std::unique_ptr<Mesh> mesh;

		AABB currentAABB, currentHitbox, lookAtAABB;

		void refillInventory();

		DrawContext* beginRenderer(bool depthTest, bool cullFace);
		void endRenderer(DrawContext* context, bool depthTest, bool cullFace);

		Shader* setupShader(const std::string& name, const glm::vec3& offset);
		Shader* setupBlocksShader(const glm::vec3& offset);
		Shader* setupEntitiesShader(const glm::vec3& offset);
		Shader* drawGridLines();
		Shader* drawDirectionArrow();
	};
}