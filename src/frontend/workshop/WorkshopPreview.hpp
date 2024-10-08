#ifndef FRONTEND_MENU_WORKSHOP_PREVIEW_HPP
#define FRONTEND_MENU_WORKSHOP_PREVIEW_HPP

#include "../../graphics/core/Batch2D.hpp"
#include "../../graphics/core/Batch3D.hpp"
#include "../../graphics/core/Framebuffer.hpp"
#include "../../graphics/core/LineBatch.hpp"
#include "../../graphics/core/Mesh.hpp"
#include "../../graphics/render/BlocksRenderer.hpp"
#include "../../graphics/ui/elements/InventoryView.hpp"
#include "../../items/Inventory.hpp"
#include "../../maths/aabb.hpp"
#include "../../window/Camera.hpp"

#include <glm/fwd.hpp>

class Engine;
class ContentGfxCache;
class Chunk;
class Level;
class World;
namespace gui {
	class UINode;
}
namespace xml {
	class Document;
}

namespace workshop {

	enum class PrimitiveType : unsigned int;

	class Preview {
	public:
		Preview(Engine* engine, ContentGfxCache* cache);

		void update(float delta, float sensitivity);
		void updateMesh();
		void updateCache();

		bool rayCast(float cursorX, float cursorY, size_t& returnIndex);
		void rotate(float x, float y);
		void scale(float value);

		void drawBlock();
		void drawUI();

		void setBlock(Block* block);
		void setCurrentAABB(const AABB& aabb, PrimitiveType type);
		void setCurrentTetragon(const glm::vec3* tetragon);

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
		PrimitiveType lookAtPrimitive;

	private:
		float refillTimer = 0.f;
		float viewDistance = 2.f;
		glm::i8vec3 blockSize;
		glm::vec2 previewRotation{ 225.f, 45.f };
		glm::vec3 currentTetragon[4]{}, lookAtTetragon[4]{};
		glm::vec3 cameraOffset, cameraPosition;
		PrimitiveType primitiveType;

		Engine* engine;
		ContentGfxCache* cache;

		BlocksRenderer blockRenderer;
		Chunk* chunk;
		World* world;
		Level* level;
		Camera camera;
		Framebuffer framebuffer;

		std::shared_ptr<Inventory> inventory;
		std::shared_ptr<gui::UINode> currentUI;
		std::shared_ptr<xml::Document> currentDocument;
		Block* currentBlock = nullptr;

		LineBatch lineBatch;
		Batch2D batch2d;
		Batch3D batch3d;
		std::shared_ptr<Mesh> mesh;

		AABB currentAABB, currentHitbox, lookAtAABB;

		void refillInventory();
	};
}

#endif // !FRONTEND_MENU_WORKSHOP_PREVIEW_HPP
