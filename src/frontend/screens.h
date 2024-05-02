#ifndef FRONTEND_SCREENS_H_
#define FRONTEND_SCREENS_H_

#include <memory>
#include "../settings.h"

#include <map>
#include <functional>
#include <glm/glm.hpp>
#include <unordered_set>
#include "../content/ContentPack.h"
#include "../maths/aabb.h"

class Assets;
class Level;
class WorldRenderer;
class Hud;
class Engine;
class Camera;
class Batch2D;
class LevelFrontend;
class LevelController;
class TextureAnimator;

class Atlas;
class Batch3D;
class Texture;
class ContentIndices;
class Content;
class Block;
class ItemDef;
class BlocksRenderer;
class Mesh;
class Chunk;
class World;
class Player;
class ContentGfxCache;
class Framebuffer;
class LineBatch;
class UiDocument;
class Inventory;
class InventoryInteraction;
struct BlockMaterial;
enum class BlockModel;
enum class item_icon_type;
namespace gui {
    class Panel;
    class GUI;
    class TextBox;
    class UINode;
    class FullCheckBox;
    class Container;
    class Button;
    class RichButton;
    class Image;
}
namespace dynamic {
    class Map;
    class List;
}
namespace scripting {
    class Environment;
}
namespace xml {
	class Node;
    class Document;
}

/* Screen is a mainloop state */
class Screen {
protected:
    Engine* engine;
    std::unique_ptr<Batch2D> batch;
public:
    Screen(Engine* engine);
    virtual ~Screen();
    virtual void update(float delta) = 0;
    virtual void draw(float delta) = 0;
    virtual void onEngineShutdown() {};
};

class MenuScreen : public Screen {
    std::unique_ptr<Camera> uicamera;
public:
    MenuScreen(Engine* engine);
    ~MenuScreen();

    void update(float delta) override;
    void draw(float delta) override;
};

class LevelScreen : public Screen {
    std::unique_ptr<LevelFrontend> frontend;
    std::unique_ptr<Hud> hud;
    std::unique_ptr<WorldRenderer> worldRenderer;
    std::unique_ptr<LevelController> controller;
    std::unique_ptr<TextureAnimator> animator;

    bool hudVisible = true;
    void updateHotkeys();
public:
    LevelScreen(Engine* engine, Level* level);
    ~LevelScreen();

    void update(float delta) override;
    void draw(float delta) override;

    void onEngineShutdown() override;

    LevelController* getLevelController() const;
};

class WorkShopScreen : public Screen {
public:
	WorkShopScreen(Engine* engine, const ContentPack& pack);
	~WorkShopScreen();

	void update(float delta) override;
	void draw(float delta) override;

	enum class DefAction {
		CREATE_NEW = 0,
		RENAME,
		DELETE
	};
	enum class DefType {
		BLOCK = 0,
		ITEM,
		BOTH,
		UI_LAYOUT
	};

private:
	bool initialize();
	void exit();

	std::shared_ptr<gui::Panel> createNavigationPanel();
	void createContentErrorMessage(ContentPack& pack, const std::string& message);
	void removePanel(unsigned int column);
	void removePanels(unsigned int column);
	void clearRemoveList(std::shared_ptr<gui::Panel> from);
	void createPanel(std::function<std::shared_ptr<gui::Panel>()> lambda, unsigned int column, float posX = -1.f);

	std::shared_ptr<gui::TextBox> createTextBox(std::shared_ptr<gui::Panel> panel, std::string& string, const std::wstring& placeholder = L"Type here");
	template<typename T>
	std::shared_ptr<gui::TextBox> createNumTextBox(T& value, const std::wstring& placeholder, T min, T max,
		const std::function<void(T)>& callback = [](T) {});
	std::shared_ptr<gui::FullCheckBox> createFullCheckBox(std::shared_ptr<gui::Panel> panel, const std::wstring& string, bool& checked);
	std::shared_ptr<gui::RichButton> createTextureButton(const std::string& texture, Atlas* atlas, glm::vec2 size, const wchar_t* side = nullptr);
	std::shared_ptr<gui::Panel> createTexturesPanel(std::shared_ptr<gui::Panel> panel, float iconSize, std::string* textures, BlockModel model);
	std::shared_ptr<gui::Panel> createTexturesPanel(std::shared_ptr<gui::Panel> panel, float iconSize, std::string& texture, item_icon_type iconType);
	std::shared_ptr<gui::UINode> createVectorPanel(glm::vec3& vec, float min, float max, float width, unsigned int inputType, const std::function<void()>& callback);
	void createEmissionPanel(std::shared_ptr<gui::Panel> panel, uint8_t* emission);
	void createAddingUIElementPanel(float posX, const std::function<void(const std::string&)>& callback, unsigned int filter);

	void createContentList(DefType type, unsigned int column = 1, bool showAll = false, const std::function<void(const std::string&)>& callback = 0, float posX = -1.f);
	void createMaterialsList(bool showAll = false, unsigned int column = 1, float posX = -1.f, const std::function<void(const std::string&)>& callback = 0);
	void createTextureList(float icoSize, unsigned int column = 1, DefType type = DefType::BOTH, float posX = -1.f, bool showAll = false,
		const std::function<void(const std::string&)>& callback = 0);
	void createScriptList(unsigned int column = 1, float posX = -1.f, const std::function<void(const std::string&)>& callback = 0);
	void createUILayoutList(bool showAll = false, unsigned int column = 1, float posX = -1.f, const std::function<void(const std::string&)>& callback = 0);
	void createSoundList();

	void createPackInfoPanel();
	void createTextureInfoPanel(const std::string& texName, DefType type);
	void createScriptInfoPanel(const fs::path& file);
	void createSoundInfoPanel(const fs::path& file);

	void createBlockEditor(Block* block);
	void createCustomModelEditor(Block* block, size_t index, unsigned int primitiveType);
	void createItemEditor(ItemDef* item);
	void createUILayoutEditor(const fs::path& path, const std::string& fullName, const std::string& actualName, std::vector<size_t> docPath);
	void createMaterialEditor(BlockMaterial& material);

	void createDefActionPanel(DefAction action, DefType type, const std::string& name = std::string(), bool reInitialize = true);
	void createImportPanel(DefType type = DefType::BLOCK, std::string mode = "copy");
	void createBlockPreview(unsigned int column, unsigned int primitiveType);
	void createUIPreview();

	void saveBlock(Block* block, const std::string& actualName) const;
	void saveItem(ItemDef* item, const std::string& actualName) const;
	void saveDocument(std::shared_ptr<xml::Document>  document, const std::string& actualName) const;

	void formatTextureImage(gui::Image* image, Atlas* atlas, float height, const std::string& texName);
	template<typename T>
	void setSelectable(std::shared_ptr<gui::Panel> panel);

	int swapInterval;
	gui::GUI* gui;
	std::map<unsigned int, std::shared_ptr<gui::Panel>> panels;
	std::unordered_map<std::string, std::shared_ptr<xml::Document>> xmlDocs;
	std::vector<std::shared_ptr<gui::UINode>> removeList;
	std::unordered_set<std::string> blocksList;
	std::unordered_set<std::string> itemsList;
	std::unique_ptr<Camera> uicamera;

	ContentPack currentPack;
	std::unique_ptr<ContentGfxCache> cache;
	const Content* content;
	ContentIndices* indices;
	Assets* assets;
	Atlas* blocksAtlas;
	Atlas* itemsAtlas;
	Atlas* previewAtlas;

	class Preview {
	private:
		std::unique_ptr<BlocksRenderer> blockRenderer;
		std::unique_ptr<Mesh> mesh;
		std::shared_ptr<Chunk> chunk;
		Level* level;
		std::unique_ptr<Camera> camera;
		World* world;
		std::unique_ptr<Framebuffer> framebuffer;
		ContentGfxCache* cache;
		Engine* engine;

		std::unique_ptr<LevelController> controller;
		std::unique_ptr<LevelFrontend> frontend;
		std::shared_ptr<Inventory> inventory;
		std::unique_ptr<InventoryInteraction> interaction;
		std::shared_ptr<gui::UINode> currentUI;

		std::unique_ptr<Batch2D> batch2d;
		std::unique_ptr<Batch3D> batch3d;
		std::unique_ptr<LineBatch> lineBatch;

		AABB currentAABB, currentHitbox;
		glm::vec3 currentTetragon[4]{};
		glm::vec2 previewRotation{ 225.f, 45.f };
		float viewDistance = 2.f;

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
		void setCurrentAABB(const AABB& aabb, unsigned int primitiveType);
		void setCurrentTetragon(const glm::vec3* tetragon);

		void setUiDocument(const std::shared_ptr<xml::Document> document, scripting::Environment* env);

		void setResolution(uint width, uint height);

		Texture* getTexture();

		bool mouseLocked = false;
		bool drawGrid = true;
		bool drawBlockBounds = false;
		bool drawCurrentAABB = false;
		bool drawCurrentTetragon = false;
		bool drawBlockHitbox = false;
		bool drawDirection = true;

	private:
		float refillTimer = 0.f;

		void refillInventory();
	};
	std::unique_ptr<Preview> preview;
};
#endif // FRONTEND_SCREENS_H_