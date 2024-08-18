#include "WorkshopUtils.hpp"

#include "../../assets/Assets.hpp"
#include "../../constants.hpp"
#include "../../content/ContentPack.hpp"
#include "../../core_defs.hpp"
#include "../../graphics/core/Atlas.hpp"
#include "../../graphics/core/Texture.hpp"
#include "../../items/ItemDef.hpp"
#include "../../maths/UVRegion.hpp"
#include "../../voxels/Block.hpp"
#include "IncludeCommons.hpp"
#include "../UiDocument.hpp"

#include <iostream>

#ifdef _WIN32
#define NOMINMAX
#include "Windows.h"
#endif // _WIN32

namespace workshop {
const std::unordered_map<std::string, UIElementInfo> uiElementsArgs {
	{ "inventory", { INVENTORY | CONTAINER, { {"size", "200, 200"} } } },
	{ "container", { CONTAINER, { {"size", "200, 200" } } } },
	{ "panel", { PANEL, { {"size", "200, 200" } } } },
	{ "image", { IMAGE, { {"src", "gui/error"} } } },
	{ "label", { LABEL | HAS_ElEMENT_TEXT | HAS_SUPPLIER, { }, { {"#", {"#", "The label"} } } } },
	{ "button", { BUTTON /*| PANEL*/ | HAS_ElEMENT_TEXT | HAS_HOVER_COLOR, {{"size", "100,20"}}, {{"#", {"#", "The button"}}}}},
	{ "textbox", { TEXTBOX /*| PANEL*/ | HAS_ElEMENT_TEXT | HAS_SUPPLIER | HAS_CONSUMER | HAS_HOVER_COLOR, { {"size", "150,40"} }, { {"#", {"#", "The textbox"} } } } },
	{ "checkbox", { CHECKBOX /*| PANEL*/ | HAS_ElEMENT_TEXT | HAS_SUPPLIER | HAS_CONSUMER | HAS_HOVER_COLOR, { {"size", "150,30"} }, { {"#", {"#", "The checkbox"} } } } },
	{ "trackbar", { TRACKBAR | HAS_SUPPLIER | HAS_CONSUMER | HAS_HOVER_COLOR,  {{"size", "150,20"}, {"max", "100"}, {"track-width", "5"}}}},
	{ "slots-grid", { SLOTS_GRID, { {"rows", "1"}, {"count", "1"} } } },
	{ "slot", { SLOT } },
	{ "bindbox", { BINDBOX /*| PANEL*/, { {"binding", "movement.left" } } } }
};
}

std::string workshop::getTexName(const std::string& fullName) {
	size_t pos = fullName.find(':');
	if (pos != std::string::npos) {
		return fullName.substr(pos + 1);
	}
	return fullName;
}

Atlas* workshop::getAtlas(Assets* assets, const std::string& fullName) {
	size_t pos = fullName.find(':');
	if (pos != std::string::npos) {
		Atlas* atlas = assets->get<Atlas>(fullName.substr(0, pos));
		if (atlas) return atlas;
	}
	return assets->get<Atlas>(BLOCKS_PREVIEW_ATLAS);
}

std::string workshop::getDefName(DefType type) {
	switch (type) {
	case DefType::BLOCK: return "block";
	case DefType::ITEM: return "item";
	case DefType::BOTH: return "[both]";
	case DefType::UI_LAYOUT: return "layout";
	}
	return "";
}

std::string workshop::getDefName(const std::string& fullName) {
	return getTexName(fullName);
}

std::string workshop::getDefFolder(DefType type) {
	switch (type) {
	case DefType::BLOCK: return ContentPack::BLOCKS_FOLDER.string();
	case DefType::ITEM: return ContentPack::ITEMS_FOLDER.string();
	case DefType::UI_LAYOUT: return LAYOUTS_FOLDER;
	}
	return "";
}

std::string workshop::getDefFileFormat(DefType type) {
	switch (type) {
	case DefType::BLOCK:
	case DefType::ITEM: return ".json";
	case DefType::UI_LAYOUT: return ".xml";
	}
	return "";
}

void workshop::formatTextureImage(gui::Image& image, Atlas* atlas, float height, const std::string& texName) {
	const UVRegion& region = atlas->get(texName);
	glm::vec2 textureSize((region.u2 - region.u1) * atlas->getTexture()->getWidth(), (region.v2 - region.v1) * atlas->getTexture()->getHeight());
	glm::vec2 multiplier(height / textureSize);
	image.setSize(textureSize * std::min(multiplier.x, multiplier.y));
	image.setPos(glm::vec2(height / 2.f - image.getSize().x / 2.f, height / 2.f - image.getSize().y / 2.f));
	image.setTexture(atlas->getTexture());
	image.setUVRegion(region);
}

template void workshop::setSelectable<gui::IconButton>(const gui::Panel&);
template void workshop::setSelectable<gui::Button>(const gui::Panel&);

template<typename T>
void workshop::setSelectable(const gui::Panel& panel) {
	for (const auto& elem : panel.getNodes()) {
		T* node = dynamic_cast<T*>(elem.get());
		if (!node) continue;
		node->listenAction([node, &panel](gui::GUI* gui) {
			for (const auto& elem : panel.getNodes()) {
				if (typeid(*elem.get()) == typeid(T)) {
					elem->setColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.95f));
					elem->setHoverColor(glm::vec4(0.05f, 0.1f, 0.15f, 0.75f));
				}
			}
			node->setColor(node->getHoverColor());
		});
	}
}

void workshop::validateBlock(Assets* assets, Block& block) {
	Atlas* blocksAtlas = assets->get<Atlas>("blocks");
	auto checkTextures = [&block, blocksAtlas](std::string* begin, std::string* end) {
		for (std::string* it = begin; it != end; it++) {
			if (blocksAtlas->has(*it)) continue;
			std::cout << "the texture \"" << *it << "\" in block \"" << block.name << "\" was removed since the texture does not exist" << std::endl;
			*it = TEXTURE_NOTFOUND;
		}
	};
	checkTextures(std::begin(block.textureFaces), std::end(block.textureFaces));
	checkTextures(block.modelTextures.data(), block.modelTextures.data() + block.modelTextures.size());
	if (block.modelBoxes.size() == block.hitboxes.size()) {
		bool removeHitboxes = true;
		for (size_t i = 0; i < block.hitboxes.size(); i++) {
			if (block.hitboxes[i].a == block.modelBoxes[i].a &&
				block.hitboxes[i].b == block.modelBoxes[i].b) continue;
			removeHitboxes = false;
			break;
		}
		if (removeHitboxes) {
			block.hitboxes.clear();
			std::cout << "removed generated hitboxes in block \"" << block.name << "\"" << std::endl;
		}
	}
}

void workshop::validateItem(Assets* assets, ItemDef& item) {
	Atlas* atlas = getAtlas(assets, item.icon);
	if (!atlas->has((item.iconType == item_icon_type::block ? item.icon : getTexName(item.icon)))) {
		std::cout << "invalid icon \"" << item.icon << "\" was reset in item \"" << item.name << "\"" << std::endl;
		item.iconType = item_icon_type::sprite;
		item.icon = "blocks:notfound";
	}
}

bool workshop::checkPackId(const std::wstring& id, const std::vector<ContentPack>& scanned) {
	return !(id.length() < 2 || id.length() > 24 || isdigit(id[0]) ||
		!std::all_of(id.begin(), id.end(), [](const wchar_t c) { return c < 255 && (iswalnum(c) || c == '_'); }) ||
		std::find(ContentPack::RESERVED_NAMES.begin(), ContentPack::RESERVED_NAMES.end(), util::wstr2str_utf8(id)) != ContentPack::RESERVED_NAMES.end() ||
		std::find_if(scanned.begin(), scanned.end(), [id](const ContentPack& pack) {
			return util::wstr2str_utf8(id) == pack.id; }
		) != scanned.end()
	);
}

bool workshop::hasFocusedTextbox(const gui::Container& container) {
	for (const auto& elem : container.getNodes()) {
		if (gui::TextBox* textBox = dynamic_cast<gui::TextBox*>(elem.get())) {
			if (textBox->isFocused()) return true;
		}
		else if (gui::Container* container = dynamic_cast<gui::Container*>(elem.get())) {
			if (hasFocusedTextbox(*container)) return true;
		}
	}
	return false;
}

std::vector<std::filesystem::path> workshop::getFiles(const std::filesystem::path& folder, bool recursive) {
	std::vector<std::filesystem::path> files;
	if (!std::filesystem::is_directory(folder)) return files;
	if (recursive) {
		for (const auto& entry : std::filesystem::recursive_directory_iterator(folder)) {
			if (entry.is_regular_file()) files.emplace_back(entry.path());
		}
	}
	else {
		for (const auto& entry : std::filesystem::directory_iterator(folder)) {
			if (entry.is_regular_file()) files.emplace_back(entry.path());
		}
	}
	return files;
}

void workshop::openPath(const std::filesystem::path& path) {
#ifdef _WIN32
	ShellExecuteW(NULL, L"open", path.c_str(), NULL, NULL, SW_SHOWDEFAULT);
#elif __linux__
	system(("nautilus " + path.string()).c_str());
#endif // WIN32
}

std::string workshop::getScriptName(const ContentPack& pack, const std::string& scriptName) {
	if (scriptName.empty()) return NOT_SET;
	else if (fs::is_regular_file(pack.folder / ("scripts/" + scriptName + ".lua")))
		return fs::relative(pack.folder / "scripts" / scriptName, pack.folder / "scripts/").string();
	return NOT_SET;
}

std::string workshop::getUILayoutName(Assets* assets, const std::string& layoutName) {
	if (layoutName.empty()) return NOT_SET;
	if (assets->get<UiDocument>(layoutName)) return layoutName;
	return NOT_SET;
}