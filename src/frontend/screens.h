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
class HudRenderer;
class Engine;
class Camera;
class Batch2D;
class LevelFrontend;
class LevelController;
class TextureAnimator;

class Atlas;
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
enum class BlockModel;
enum class item_icon_type;
namespace gui {
    class Panel;
    class GUI;
    class TextBox;
    class UINode;
    class FullCheckBox;
    class Container;
    class RichButton;
    class Image;
}
namespace dynamic {
    class Map;
    class List;
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
    std::unique_ptr<Level> level;
    std::unique_ptr<LevelFrontend> frontend;
    std::unique_ptr<HudRenderer> hud;
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

    Level* getLevel() const;
};

class WorkShopScreen : public Screen {
private:
    gui::GUI* gui;
    std::map<unsigned int, std::shared_ptr<gui::Panel>> panels;
    std::vector<std::shared_ptr<gui::UINode>> removeList;
    std::unordered_set<std::string> blocksList;
    std::unordered_set<std::string> itemsList;
    std::unique_ptr<Camera> uicamera;

    ContentPack currentPack;
    std::string currentBlockIco{ "blocks:notfound" };
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
        std::unique_ptr<Level> level;
        std::unique_ptr<LineBatch> lineBatch;
        std::unique_ptr<Camera> camera;
        World* world;
        Player* player;
        std::unique_ptr<Framebuffer> framebuffer;
        ContentGfxCache* cache;
        Engine* engine;

        AABB blockHitbox;
        AABB currentAABB;
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

        void draw();

        void setBlock(Block* block);
        void setCurrentAABB(const AABB& aabb) { currentAABB.a = aabb.a + (aabb.b - aabb.a) / 2.f - 0.5f;
                                                currentAABB.b = aabb.b - aabb.a; };
        void setCurrentTetragon(const glm::vec3* tetragon);
        void setBlockHitbox(const AABB& aabb) { blockHitbox.a = aabb.a + (aabb.b - aabb.a) / 2.f - 0.5f;
                                                blockHitbox.b = aabb.b - aabb.a; };
        void setResolution(uint width, uint height);

        Texture* getTexture();

        bool mouseLocked = false;
        bool drawGrid = true;
        bool drawBlockBounds = false;
        bool drawCurrentAABB = false;
        bool drawCurrentTetragon = false;
        bool drawBlockHitbox = false;
    };
    std::unique_ptr<Preview> preview;
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
        BOTH
    };
private:
    void initialize();
    void removePanel(unsigned int column);
    void removePanels(unsigned int column);
    void clearRemoveList(std::shared_ptr<gui::Panel> from);
    void createPanel(std::function<std::shared_ptr<gui::Panel>()> lambda, unsigned int column, float posX = -1.f);
    std::shared_ptr<gui::TextBox> createTextBox(std::shared_ptr<gui::Panel> panel, std::string& string, const std::wstring& placeholder = L"Type here");
    template<typename T>
    std::shared_ptr<gui::TextBox> createNumTextBox(T& value, const std::wstring& placeholder, T min, T max,
        const std::function<void(T)>& callback = [](T) {});
    std::shared_ptr<gui::FullCheckBox> createFullCheckBox(std::shared_ptr<gui::Panel> panel, const std::wstring& string, bool& checked);
    std::shared_ptr<gui::Container> createHitboxContainer(AABB& aabb, float width, const std::function<void(float)>& callback);
    std::shared_ptr<gui::RichButton> createTextureButton(const std::string& texture, Atlas* atlas, glm::vec2 size, const wchar_t* side = nullptr);
    std::shared_ptr<gui::Panel> createTexturesPanel(glm::vec2 size, std::string* textures, BlockModel model);
    std::shared_ptr<gui::Panel> createTexturesPanel(glm::vec2 size, std::string& texture, item_icon_type model);
    void createEmissionPanel(std::shared_ptr<gui::Panel> panel, uint8_t* emission);

    void createContentList(DefType type, unsigned int column, bool showAll = false, const std::function<void(const std::string&)>& callback = 0);
    void createInfoPanel();
    void createTextureList(float icoSize, unsigned int column, DefType type = DefType::BOTH, float posX = -1.f, bool showAll = false,
        const std::function<void(const std::string&)>& callback = 0);
    void createDefActionPanel(DefAction action, DefType type, const std::string& name = std::string());
    void createTextureInfoPanel(const std::string& texName, DefType type);
    void createImportPanel(DefType type = DefType::BLOCK);
    void createBlockEditor(const std::string& blockName);
    void createItemEditor(const std::string& itemName);
    void createCustomModelEditor(Block* block, size_t index, unsigned int primitiveType);
    void createPreview(unsigned int column, unsigned int primitiveType);
    void saveBlock(Block* block, const std::string& actualName) const;
    void saveItem(ItemDef* item, const std::string& actualName) const;

    void formatTextureImage(gui::Image* image, Atlas* atlas, float height, const std::string& texName);
    template<typename T>
    void setNodeColor(std::shared_ptr<gui::Panel> panel) {
		for (const auto& elem : panel->getNodes()) {
            if (typeid(*elem.get()) != typeid(T)) continue;
            T* node = static_cast<T*>(elem.get());
            node->listenAction([node, panel](gui::GUI* gui) {
				for (const auto& elem : panel->getNodes()) {
					if (typeid(*elem.get()) == typeid(T)) {
						elem->setColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.95f));
						elem->setHoverColor(glm::vec4(0.05f, 0.1f, 0.15f, 0.75f));
					}
				}
                node->setColor(node->getHoverColor());
			});
		}
    };
};
#endif // FRONTEND_SCREENS_H_