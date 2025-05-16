#pragma once

#include "frontend/screens/Screen.hpp"
#include "content/ContentPack.hpp"
#include "WorkshopUtils.hpp"

#include <filesystem>
#include <functional>
#include <glm/glm.hpp>
#include <map>
#include <unordered_map>

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
	class GUI;
}
namespace xml {
	class Document;
}
namespace data{
	class StructLayout;
}
enum class BlockModel;
enum class ItemIconType;

namespace workshop {
	class Preview;
	struct BackupData {
		std::string string;
		std::string currentParent;
		std::string newParent;
	};

	class WorkShopScreen : public Screen {
	public:
		WorkShopScreen(Engine& engine, const ContentPack pack);
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

		void createTextureList(float iconSize, unsigned int column = 1, std::vector<ContentType> types = { ContentType::BLOCK, ContentType::ITEM },
			float posX = PANEL_POSITION_AUTO, bool showAll = false, const std::function<void(const std::string&)>& callback = 0);
		void createTextureInfoPanel(const std::string& texName, ContentType type, unsigned int column = 2);
		void createTexturesPanel(gui::Panel& panel, float iconSize, std::string* textures, BlockModel model, const std::function<void()>& callback = 0);
		void createTexturesPanel(gui::Panel& panel, float iconSize, std::string& texture, ItemIconType iconType);

		void createContentList(ContentType type, bool showAll = false, unsigned int column = 1, float posX = PANEL_POSITION_AUTO, const std::function<void(const std::string&)>& callback = 0);
		void createMaterialsList(bool showAll = false, unsigned int column = 1, float posX = PANEL_POSITION_AUTO, const std::function<void(const std::string&)>& callback = 0);
		void createScriptList(unsigned int column = 1, float posX = PANEL_POSITION_AUTO, const std::function<void(const std::string&)>& callback = 0);
		void createUILayoutList(bool showAll = false, unsigned int column = 1, float posX = PANEL_POSITION_AUTO, const std::function<void(const std::string&)>& callback = 0);
		void createAddingUIElementPanel(float posX, const std::function<void(const std::string&)>& callback, unsigned int filter);
		void createSoundList();
		void createEntitiesList(bool showAll = false, unsigned int column = 1, float posX = PANEL_POSITION_AUTO, const std::function<void(const std::string&)>& callback = 0);
		void createSkeletonList(bool showAll = false, unsigned int column = 1, float posX = PANEL_POSITION_AUTO, const std::function<void(const std::string&)>& callback = 0);
		void createModelsList(bool showAll = false, unsigned int column = 1, float posX = PANEL_POSITION_AUTO, const std::function<void(const std::string&)>& callback = 0);

		void createPackInfoPanel();
		void createScriptInfoPanel(const std::filesystem::path& file);
		void createSoundInfoPanel(const std::filesystem::path& file);
		void createSettingsPanel();
		void createUtilsPanel();

		void createBlockEditor(Block& block);
		void createAdditionalBlockEditorPanel(Block& block, size_t index, PrimitiveType type);
		void createBlockConverterPanel(Block& block, float posX);
		void createPrimitiveEditor(gui::Panel& panel, Block& block, size_t index, PrimitiveType type);
		void createPropertyEditor(gui::Panel& panel, dv::value& blockProps, const dv::value& definedProps, std::vector<size_t> path = {});
		void createBlockFieldsEditor(gui::Panel& panel, std::unique_ptr<data::StructLayout>& fields);
		gui::Panel& createBlockPreview(gui::Panel& parentPanel, Block& block, PrimitiveType type);

		void createItemEditor(ItemDef& item);
		void createEntityEditorPanel(EntityDef& entity);
		void createAdditionalEntityEditorPanel(EntityDef& entity, unsigned int mode);
		void createSkeletonEditorPanel(rigging::SkeletonConfig& skeleton, std::vector<size_t> skeletonPath);
		void createUILayoutEditor(const std::filesystem::path& path, const std::string& fullName, std::vector<size_t> docPath);
		void createMaterialEditor(BlockMaterial& material);

		void createDefActionPanel(ContentAction action, ContentType type, const std::string& name = std::string(), bool reInitialize = true);
		void createImportPanel(ContentType type = ContentType::BLOCK, std::string mode = "copy");
		void createFileDeletingConfirmationPanel(const std::vector<std::filesystem::path>& files, unsigned int column, const std::function<void(void)>& callback);

		gui::Panel& createPreview(const std::function<void(gui::Panel&, gui::Image&)>& setupFunc);
		void createSkeletonPreview(unsigned int column);
		void createModelPreview(unsigned int column);
		void createUIPreview();

		void backupDefs();
		bool showUnsaved(const std::function<void()>& callback = 0);

		bool ignoreUnsaved = false;
		int framerate;
		PrimitiveType modelPrimitive = PrimitiveType::AABB;

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
		dv::value definedProps;

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