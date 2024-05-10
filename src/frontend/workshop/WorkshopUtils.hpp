#ifndef FRONTEND_MENU_WORKSHOP_UTILS_HPP
#define FRONTEND_MENU_WORKSHOP_UTILS_HPP

#include <filesystem>
#include <set>
#include <string>
#include <unordered_map>

class Assets;
class Atlas;
class Block;
class ItemDef;
struct ContentPack;
namespace gui {
	class Image;
	class Panel;
	class Container;
}

inline const std::string NOT_SET = "[not set]";
inline const std::string BLOCKS_PREVIEW_ATLAS = "blocks_preview";
inline constexpr float PANEL_POSITION_AUTO = std::numeric_limits<float>::min();

namespace workshop {
	enum class PrimitiveType : unsigned int {
		AABB = 0,
		TETRAGON,
		HITBOX,

		COUNT
	};

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

	enum UIElementsArgs : unsigned long long {
		INVENTORY = 0x1ULL, CONTAINER = 0x2ULL, PANEL = 0x4ULL, IMAGE = 0x8ULL, LABEL = 0x10ULL, BUTTON = 0x20ULL, TEXTBOX = 0x40ULL, CHECKBOX = 0x80ULL,
		TRACKBAR = 0x100ULL, SLOTS_GRID = 0x200ULL, SLOT = 0x400ULL, /* 11 bits used, 6 bits reserved */
		HAS_ElEMENT_TEXT = 0x10000ULL, HAS_CONSUMER = 0x20000ULL, HAS_SUPPLIER = 0x40000ULL, HAS_HOVER_COLOR = 0x80000ULL,
	};

	struct UIElementInfo {
		unsigned long long args;
		std::vector<std::pair<std::string, std::string>> attrTemplate;
		std::vector<std::pair<std::string, std::pair<std::string, std::string>>> elemsTemplate;
	};

	extern const std::unordered_map<std::string, UIElementInfo> uiElementsArgs;

	extern Atlas* getAtlas(Assets* assets, const std::string& fullName);

	extern std::string getTexName(const std::string& fullName);
	extern std::string getDefName(DefType type);
	extern std::string getScriptName(const ContentPack& pack, const std::string& scriptName);
	extern std::string getUILayoutName(Assets* assets, const std::string& layoutName);

	extern std::string getDefFolder(DefType type);
	extern std::string getDefFileFormat(DefType type);

	extern void formatTextureImage(gui::Image* image, Atlas* atlas, float height, const std::string& texName);
	template<typename T>
	extern void setSelectable(std::shared_ptr<gui::Panel> panel);

	extern void validateBlock(Assets* assets, Block& block);
	extern void validateItem(Assets* assets, ItemDef& item);

	extern bool hasFocusedTextbox(const std::shared_ptr<gui::Container> container);

	extern std::set<std::filesystem::path> getFiles(const std::filesystem::path& folder, bool recursive);
}
#endif // !FRONTEND_MENU_WORKSHOP_UTILS_HPP
