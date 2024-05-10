#ifndef FRONTEND_MENU_WORKSHOP_PREVIEW_HPP
#define FRONTEND_MENU_WORKSHOP_PREVIEW_HPP

#include "../../graphics/Batch2D.h"
#include "../../graphics/Batch3D.h"
#include "../../graphics/Framebuffer.h"
#include "../../graphics/LineBatch.h"
#include "../../graphics/Mesh.h"
#include "../../items/Inventory.h"
#include "../../logic/LevelController.h"
#include "../../maths/aabb.h"
#include "../../window/Camera.h"
#include "../graphics/BlocksRenderer.h"
#include "../InventoryView.h"
#include "../LevelFrontend.h"

#include <glm\fwd.hpp>

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
namespace scripting {
	class Environment;
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

		void setUiDocument(const std::shared_ptr<xml::Document> document, scripting::Environment* env, bool forseUpdate = false);

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
		Mesh mesh;

		LevelController controller;
		LevelFrontend frontend;
		std::shared_ptr<Inventory> inventory;
		InventoryInteraction interaction;
		std::shared_ptr<gui::UINode> currentUI;
		std::shared_ptr<xml::Document> currentDocument;

		Batch2D batch2d;
		Batch3D batch3d;
		LineBatch lineBatch;

		AABB currentAABB, currentHitbox;

		void refillInventory();
	};
}

#endif // !FRONTEND_MENU_WORKSHOP_PREVIEW_HPP
