#ifndef FRONTEND_MENU_WORKSHOP_SCREEN_HPP
#define FRONTEND_MENU_WORKSHOP_SCREEN_HPP

#include "../screens/Screen.hpp"

#include "content/ContentPack.hpp"
#include "WorkshopUtils.hpp"

#include <filesystem>
#include <functional>
#include <glm/glm.hpp>
#include <map>
#include <unordered_map>
#include <unordered_set>

class Atlas;
class Block;
class Camera;
class Content;
class ContentIndices;
class ContentGfxCache;
struct ItemDef;
struct EntityDef;
struct BlockMaterial;
namespace rigging {
	class SkeletonConfig;
}
namespace gui {
	class Panel;
	class UINode;
	class GUI;
}
namespace xml {
	class Document;
}
enum class BlockModel;
enum class item_icon_type;


namespace workshop {
	class Preview;
	struct BackupData {
		std::string string;
		std::string currentParent;
		std::string newParent;
	};

	class WorkShopScreen : public Screen {
	public:
		WorkShopScreen(Engine* engine, const ContentPack& pack);
		~WorkShopScreen();

		void update(float delta) override;
		void draw(float delta) override;

	private:
		bool initialize();
		void exit();

		void createNavigationPanel();
		void createContentErrorMessage(ContentPack& pack, const std::string& message);
		void removePanel(unsigned int column);
		void removePanels(unsigned int column);
		void createPanel(const std::function<gui::Panel& ()>& lambda, unsigned int column, float posX = PANEL_POSITION_AUTO);

		void createTexturesPanel(gui::Panel& panel, float iconSize, std::string* textures, BlockModel model);
		void createTexturesPanel(gui::Panel& panel, float iconSize, std::string& texture, item_icon_type iconType);

		void createAddingUIElementPanel(float posX, const std::function<void(const std::string&)>& callback, unsigned int filter);

		void createContentList(ContentType type, bool showAll = false, unsigned int column = 1, float posX = PANEL_POSITION_AUTO, const std::function<void(const std::string&)>& callback = 0);
		void createMaterialsList(bool showAll = false, unsigned int column = 1, float posX = PANEL_POSITION_AUTO, const std::function<void(const std::string&)>& callback = 0);
		void createTextureList(float iconSize, unsigned int column = 1, std::vector<ContentType> types = { ContentType::BLOCK, ContentType::ITEM },
			float posX = PANEL_POSITION_AUTO, bool showAll = false, const std::function<void(const std::string&)>& callback = 0);
		void createScriptList(unsigned int column = 1, float posX = PANEL_POSITION_AUTO, const std::function<void(const std::string&)>& callback = 0);
		void createUILayoutList(bool showAll = false, unsigned int column = 1, float posX = PANEL_POSITION_AUTO, const std::function<void(const std::string&)>& callback = 0);
		void createSoundList();
		void createEntitiesList(bool showAll = false, unsigned int column = 1, float posX = PANEL_POSITION_AUTO, const std::function<void(const std::string&)>& callback = 0);
		void createSkeletonList(bool showAll = false, unsigned int column = 1, float posX = PANEL_POSITION_AUTO, const std::function<void(const std::string&)>& callback = 0);
		void createModelsList(bool showAll = false, unsigned int column = 1, float posX = PANEL_POSITION_AUTO, const std::function<void(const std::string&)>& callback = 0);

		void createPackInfoPanel();
		void createTextureInfoPanel(const std::string& texName, ContentType type, unsigned int column = 2);
		void createScriptInfoPanel(const std::filesystem::path& file);
		void createSoundInfoPanel(const std::filesystem::path& file);
		void createSettingsPanel();
		void createUtilsPanel();

		void createBlockEditor(Block& block);
		void createCustomModelEditor(Block& block, size_t index, PrimitiveType type);
		void createItemEditor(ItemDef& item);
		void createEntityEditorPanel(EntityDef& entity);
		void createSkeletonEditorPanel(rigging::SkeletonConfig& skeleton, std::vector<size_t> skeletonPath);
		void createAdditionalEntityEditorPanel(EntityDef& entity, unsigned int mode);
		void createUILayoutEditor(const std::filesystem::path& path, const std::string& fullName, std::vector<size_t> docPath);
		void createMaterialEditor(BlockMaterial& material);
		void createBlockConverterPanel(Block& block, float posX);

		void createDefActionPanel(ContentAction action, ContentType type, const std::string& name = std::string(), bool reInitialize = true);
		void createImportPanel(ContentType type = ContentType::BLOCK, std::string mode = "copy");
		void createFileDeletingConfirmationPanel(const std::vector<std::filesystem::path>& files, unsigned int column, const std::function<void(void)>& callback);
		void createPreview(unsigned int column, const std::function<void(gui::Panel&, gui::Image&)>& setupFunc);
		void createBlockPreview(unsigned int column, PrimitiveType primitiveType, Block& block);
		void createSkeletonPreview(unsigned int column);
		void createModelPreview(unsigned int column);
		void createUIPreview();

		void backupDefs();
		bool showUnsaved(const std::function<void()>& callback = 0);

		bool ignoreUnsaved = false;
		int framerate;
		gui::GUI* gui;
		std::map<unsigned int, std::shared_ptr<gui::Panel>> panels;
		std::unordered_map<std::string, std::shared_ptr<xml::Document>> xmlDocs;
		std::unordered_map<std::string, BackupData> blocksList;
		std::unordered_map<std::string, BackupData> itemsList;
		std::unordered_map<std::string, BackupData> entityList;
		std::unordered_map<std::string, std::string> skeletons;
		std::unique_ptr<Camera> uicamera;

		ContentPack currentPack;
		std::string currentPackId;
		std::unique_ptr<ContentGfxCache> cache;
		const Content* content;
		ContentIndices* indices;
		Assets* assets;
		Atlas* blocksAtlas;
		Atlas* itemsAtlas;
		Atlas* previewAtlas;

		struct {
			float previewSensitivity = 1.f;
			float contentListWidth = 250.f;
			float blockEditorWidth = 250.f;
			float customModelEditorWidth = 250.f;
			float itemEditorWidth = 250.f;
			float entityEditorWidth = 250.f;
			float textureListWidth = 250.f;
			glm::vec2 customModelRange{ 0.f, 1.f };
		} settings;

		std::unique_ptr<Preview> preview;
	};
}

#endif // !FRONTEND_MENU_WORKSHOP_SCREEN_HPP