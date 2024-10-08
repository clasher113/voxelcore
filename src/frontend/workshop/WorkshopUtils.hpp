#ifndef FRONTEND_MENU_WORKSHOP_UTILS_HPP
#define FRONTEND_MENU_WORKSHOP_UTILS_HPP

#include <filesystem>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>

class Assets;
class Atlas;
class Block;
struct ItemDef;
struct ContentPack;
struct UVRegion;
struct AABB;
namespace gui {
	class Image;
	class Panel;
	class Container;
	class UINode;
}

inline const std::string NOT_SET = "[not set]";
inline const std::string BLOCKS_PREVIEW_ATLAS = "block-previews";
inline const std::string REMOVABLE_MARK = "removable";
inline const std::filesystem::path SETTINGS_FILE("worlds/workshop_settings.json");
inline constexpr float PANEL_POSITION_AUTO = std::numeric_limits<float>::min();

namespace workshop {
	enum class PrimitiveType : unsigned int {
		AABB = 0, TETRAGON, HITBOX, COUNT
	};

	enum class ContentAction {
		CREATE_NEW = 0, RENAME, DELETE
	};

	enum class ContentType {
		BLOCK = 0, ITEM, BOTH, UI_LAYOUT, ENTITY
	};

	enum UIElementsArgs : unsigned long long {
		INVENTORY = 0x1ULL, CONTAINER = 0x2ULL, PANEL = 0x4ULL, IMAGE = 0x8ULL, LABEL = 0x10ULL, BUTTON = 0x20ULL, TEXTBOX = 0x40ULL, CHECKBOX = 0x80ULL,
		TRACKBAR = 0x100ULL, SLOTS_GRID = 0x200ULL, SLOT = 0x400ULL, BINDBOX = 0x800ULL, /* 12 bits used, 5 bits reserved */
		HAS_ElEMENT_TEXT = 0x10000ULL, HAS_CONSUMER = 0x20000ULL, HAS_SUPPLIER = 0x40000ULL, HAS_HOVER_COLOR = 0x80000ULL,
	};

	struct UIElementInfo {
		unsigned long long args;
		std::vector<std::pair<std::string, std::string>> attrTemplate;
		std::vector<std::pair<std::string, std::pair<std::string, std::string>>> elemsTemplate;
	};

	extern const std::unordered_map<std::string, UIElementInfo> uiElementsArgs;

	extern Atlas* getAtlas(Assets* assets, const std::string& fullName, const std::string& delimiter = ":");

	extern std::string getTexName(const std::string& fullName, const std::string& delimiter = ":");
	extern std::string getDefName(ContentType type);
	extern std::string getDefName(const std::string& fullName);
	extern std::string getScriptName(const ContentPack& pack, const std::string& scriptName);
	extern std::string getUILayoutName(Assets* assets, const std::string& layoutName);
	extern std::string getPrimitiveName(PrimitiveType type);

	extern std::string getDefFolder(ContentType type);
	extern std::string getDefFileFormat(ContentType type);

	extern bool operator==(const UVRegion& left, const UVRegion& right);
	extern gui::Container& operator+=(gui::Container& left, gui::UINode& right);
	extern gui::Container& operator+=(gui::Container& left, gui::UINode* right);
	extern bool operator==(const AABB& left, const AABB& right);

	extern std::vector<glm::vec3> aabb2tetragons(const AABB& aabb);

	extern void formatTextureImage(gui::Image& image, const Atlas* atlas, float height, const std::string& texName);
	template<typename T>
	extern void setSelectable(const gui::Panel& panel);
	extern void placeNodesHorizontally(gui::Container& container);
	extern void optimizeContainer(gui::Container& container);
	extern gui::UINode* markRemovable(gui::UINode* node);
	extern gui::UINode& markRemovable(gui::UINode& node);
	extern void removeRemovable(gui::Container& container);

	extern void validateBlock(Assets* assets, Block& block);
	extern void validateItem(Assets* assets, ItemDef& item);
	extern bool checkPackId(const std::wstring& id, const std::vector<ContentPack>& scanned);

	extern bool hasFocusedTextbox(const gui::Container& container);

	extern std::vector<std::filesystem::path> getFiles(const std::filesystem::path& folder, bool recursive);
	extern void openPath(const std::filesystem::path& path);

	template<typename T>
	extern T incrementEnumClass(T enumClass, int factor) {
		return static_cast<T>(static_cast<int>(enumClass) + factor);
	}
}
#endif // !FRONTEND_MENU_WORKSHOP_UTILS_HPP
