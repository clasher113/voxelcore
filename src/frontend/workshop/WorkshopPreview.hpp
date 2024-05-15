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
#include "../../logic/LevelController.hpp"
#include "../../maths/aabb.hpp"
#include "../../window/Camera.hpp"
#include "../LevelFrontend.hpp"

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

		void update(float delta);
		void updateMesh();
		void updateCache();

		void rotate(float x, float y);
		void scale(float value);

		void drawBlock();
		void drawUI();

		void setBlock(Block* block);
		void setCurrentAABB(const AABB& aabb, PrimitiveType type);
		void setCurrentTetragon(const glm::vec3* tetragon);

		void setUiDocument(const std::shared_ptr<xml::Document> document, std::shared_ptr<int> enviroment, bool forseUpdate = false);

		void setResolution(uint width, uint height);

		Texture* getTexture();

		bool lockedKeyboardInput = false;
		bool mouseLocked = false;
		bool drawGrid = true;
		bool drawBlockBounds = false;
		bool drawCurrentAABB = false;
		bool drawCurrentTetragon = false;
		bool drawBlockHitbox = false;
		bool drawDirection = true;

	private:
		float refillTimer = 0.f;
		float viewDistance = 2.f;
		glm::vec2 previewRotation{ 225.f, 45.f };
		glm::vec3 currentTetragon[4]{};

		Engine* engine;
		ContentGfxCache* cache;

		BlocksRenderer blockRenderer;
		Chunk* chunk;
		World* world;
		Level* level;
		Camera camera;
		Framebuffer framebuffer;

		LevelController controller;
		LevelFrontend frontend;
		std::shared_ptr<Inventory> inventory;
		std::shared_ptr<gui::UINode> currentUI;
		std::shared_ptr<xml::Document> currentDocument;
		Block* currentBlock = nullptr;

		LineBatch lineBatch;
		Batch2D batch2d;
		Batch3D batch3d;
		Mesh mesh;

		AABB currentAABB, currentHitbox;

		void refillInventory();
	};
}

#endif // !FRONTEND_MENU_WORKSHOP_PREVIEW_HPP
