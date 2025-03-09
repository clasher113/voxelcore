#include "WorkshopUtils.hpp"

#include "../../assets/Assets.hpp"
#include "../../constants.hpp"
#include "../../content/ContentPack.hpp"
#include "../../core_defs.hpp"
#include "../../debug/Logger.hpp"
#include "../../graphics/core/Atlas.hpp"
#include "../../graphics/core/Texture.hpp"
#include "../../items/ItemDef.hpp"
#include "../../maths/UVRegion.hpp"
#include "../../voxels/Block.hpp"
#include "../UiDocument.hpp"
#include "IncludeCommons.hpp"

#ifdef _WIN32
#define NOMINMAX
#include "Windows.h"
#endif // _WIN32

static debug::Logger logger("workshop-validator");

namespace workshop {
	const std::unordered_map<std::string, UIElementInfo> uiElementsArgs{
		{ "inventory", { INVENTORY | CONTAINER, { { "size", "200, 200" } } } },
		{ "container", { CONTAINER, { { "size", "200, 200" } } } },
		{ "panel", { PANEL, { { "size", "200, 200" } } } },
		{ "image", { IMAGE, { { "src", "gui/error" } } } },
		{ "label", { LABEL | HAS_ElEMENT_TEXT | HAS_SUPPLIER, {}, { { "#", { "#", "The label" } } } } },
		{ "button", { BUTTON /*| PANEL*/ | HAS_ElEMENT_TEXT | HAS_HOVER_COLOR, { { "size", "100,20" } }, { { "#", { "#", "The button" } } } } },
		{ "textbox", { TEXTBOX /*| PANEL*/ | HAS_ElEMENT_TEXT | HAS_SUPPLIER | HAS_CONSUMER | HAS_HOVER_COLOR, { { "size", "150,40" } }, { { "#", { "#", "The textbox" } } } } },
		{ "checkbox", { CHECKBOX /*| PANEL*/ | HAS_ElEMENT_TEXT | HAS_SUPPLIER | HAS_CONSUMER | HAS_HOVER_COLOR, { { "size", "150,30" } }, { { "#", { "#", "The checkbox" } } } } },
		{ "trackbar", { TRACKBAR | HAS_SUPPLIER | HAS_CONSUMER | HAS_HOVER_COLOR, { { "size", "150,20" }, { "max", "100" }, { "track-width", "5" } } } },
		{ "slots-grid", { SLOTS_GRID, { { "rows", "1" }, { "count", "1" } } } },
		{ "slot", { SLOT } },
		{ "bindbox", { BINDBOX /*| PANEL*/, { { "binding", "movement.left" } } } }
	};
}

std::string workshop::getTexName(const std::string& fullName, const std::string& delimiter) {
	const size_t pos = fullName.find(delimiter);
	if (pos != std::string::npos) {
		return fullName.substr(pos + 1);
	}
	return fullName;
}

Atlas* workshop::getAtlas(Assets* assets, const std::string& fullName, const std::string& delimiter) {
	const size_t pos = fullName.find(delimiter);
	if (pos != std::string::npos) {
		Atlas* atlas = assets->get<Atlas>(fullName.substr(0, pos));
		if (atlas) return atlas;
	}
	return assets->get<Atlas>(BLOCKS_PREVIEW_ATLAS);
}

std::string workshop::getDefName(ContentType type) {
	const char* const names[] = { "block", "item", "layout", "entity", "skeleton", "model" };
	if (static_cast<size_t>(type) >= std::size(names)) return "";
	return names[static_cast<size_t>(type)];
}

std::string workshop::getDefName(const std::string& fullName) {
	return getTexName(fullName, ":");
}

std::string workshop::getDefFolder(ContentType type) {
	const std::string folders[] = { ContentPack::BLOCKS_FOLDER.string(), ContentPack::ITEMS_FOLDER.string(), LAYOUTS_FOLDER, 
		ContentPack::ENTITIES_FOLDER.string(), SKELETONS_FOLDER, MODELS_FOLDER };
	if (static_cast<size_t>(type) >= std::size(folders)) return "";
	return folders[static_cast<size_t>(type)];
}

std::string workshop::getDefFileFormat(ContentType type) {
	switch (type) {
		case ContentType::BLOCK:
		case ContentType::ENTITY:
		case ContentType::SKELETON:
		case ContentType::ITEM: return ".json";
		case ContentType::UI_LAYOUT: return ".xml";
		case ContentType::MODEL: return ".obj";
	}
	return "";
}

std::filesystem::path workshop::getConfigFolder(Engine& engine) {
	return engine.getPaths().getConfigFolder() / "workshop-mod";
}

bool workshop::operator==(const UVRegion& left, const UVRegion& right) {
	return left.u1 == right.u1 && left.u2 == right.u2 && left.v1 == right.v1 && left.v2 == right.v2;
}

gui::Container& workshop::operator<<(gui::Container& left, gui::UINode& right) {
	return left << &right;
}

gui::Container& workshop::operator<<(gui::Container& left, gui::UINode* right) {
	left.add(std::shared_ptr<gui::UINode>(right));
	return left;
}

bool workshop::operator==(const AABB& left, const AABB& right) {
	return left.a == right.a && left.b == right.b;
}

std::vector<glm::vec3> workshop::aabb2tetragons(const AABB& aabb) {
	std::vector<glm::vec3> result;

	// east (left)
	result.emplace_back(aabb.b.x, aabb.a.y, aabb.b.z);
	result.emplace_back(aabb.b.x, aabb.a.y, aabb.a.z);
	result.emplace_back(aabb.b.x, aabb.b.y, aabb.a.z);
	result.emplace_back(aabb.b.x, aabb.b.y, aabb.b.z);

	// west (right)
	result.emplace_back(aabb.a.x, aabb.a.y, aabb.a.z);
	result.emplace_back(aabb.a.x, aabb.a.y, aabb.b.z);
	result.emplace_back(aabb.a.x, aabb.b.y, aabb.b.z);
	result.emplace_back(aabb.a.x, aabb.b.y, aabb.a.z);

	// down (bottom)
	result.emplace_back(aabb.a.x, aabb.a.y, aabb.a.z);
	result.emplace_back(aabb.b.x, aabb.a.y, aabb.a.z);
	result.emplace_back(aabb.b.x, aabb.a.y, aabb.b.z);
	result.emplace_back(aabb.a.x, aabb.a.y, aabb.b.z);

	// up (top)
	result.emplace_back(aabb.a.x, aabb.b.y, aabb.b.z);
	result.emplace_back(aabb.b.x, aabb.b.y, aabb.b.z);
	result.emplace_back(aabb.b.x, aabb.b.y, aabb.a.z);
	result.emplace_back(aabb.a.x, aabb.b.y, aabb.a.z);

	// south (back)
	result.emplace_back(aabb.a.x, aabb.a.y, aabb.b.z);
	result.emplace_back(aabb.b.x, aabb.a.y, aabb.b.z);
	result.emplace_back(aabb.b.x, aabb.b.y, aabb.b.z);
	result.emplace_back(aabb.a.x, aabb.b.y, aabb.b.z);

	// north (front)
	result.emplace_back(aabb.b.x, aabb.a.y, aabb.a.z);
	result.emplace_back(aabb.a.x, aabb.a.y, aabb.a.z);
	result.emplace_back(aabb.a.x, aabb.b.y, aabb.a.z);
	result.emplace_back(aabb.b.x, aabb.b.y, aabb.a.z);

	return result;
}

void workshop::formatTextureImage(gui::Image& image, Texture* const texture, float height, const UVRegion& region) {
	const glm::vec2 textureSize((region.u2 - region.u1) * texture->getWidth(), (region.v2 - region.v1) * texture->getHeight());
	const glm::vec2 multiplier(height / textureSize);
	image.setSize(textureSize * std::min(multiplier.x, multiplier.y));
	image.setPos(glm::vec2(height / 2.f - image.getSize().x / 2.f, height / 2.f - image.getSize().y / 2.f));
	image.setTexture(texture);
	image.setUVRegion(region);
}

template void workshop::setSelectable<gui::IconButton>(const gui::Panel&);
template void workshop::setSelectable<gui::Button>(const gui::Panel&);

template<typename T>
void workshop::setSelectable(const gui::Panel& panel) {
	for (const auto& elem : panel.getNodes()) {
		if (T* node = dynamic_cast<T*>(elem.get())) {
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
}

void workshop::placeNodesHorizontally(gui::Container& container) {
	auto& nodes = container.getNodes();
	const float width = container.getSize().x / nodes.size();
	float height = 0.f;
	size_t i = 0;
	for (auto& elem : nodes) {
		elem->setSize(glm::vec2(width - elem->getMargin().x - 4, elem->getSize().y));
		elem->setPos(glm::vec2(width * i++, 0.f));
		height = elem->getSize().y;
	}
	container.setSize(glm::vec2(container.getSize().x, height));
	container.setScrollable(false);
}

void workshop::optimizeContainer(gui::Container& container) {
	container.listenInterval(0.01f, [&container]() {
		const glm::vec2 pos = container.calcPos();
		const glm::vec2 size = container.getSize();
		for (const auto& node : container.getNodes()) {
			const glm::vec2 nodePos = node->calcPos();
			const glm::vec2 nodeSize = node->getSize();
			node->setVisible(nodePos.y + nodeSize.y * 2.f > pos.y && nodePos.y - nodeSize.y * 2.f < pos.y + size.y);
		}
	});
}

gui::UINode* workshop::markRemovable(gui::UINode* node) {
	node->setId(REMOVABLE_MARK);
	return node;
}

gui::UINode& workshop::markRemovable(gui::UINode& node) {
	return *markRemovable(&node);
}

void workshop::removeRemovable(gui::Container& container) {
	for (int i = container.getNodes().size() - 1; i >= 0; i--) {
		auto& node = container.getNodes().at(i);
		if (node->getId() == REMOVABLE_MARK) container.remove(node);
	}
}

void workshop::validateBlock(Assets* assets, Block& block) {
	Atlas* blocksAtlas = assets->get<Atlas>("blocks");
	auto checkTextures = [&block, blocksAtlas](std::string* begin, std::string* end) {
		for (std::string* it = begin; it != end; it++) {
			if (blocksAtlas->has(*it)) continue;
			logger.info() << "the texture \"" << *it << "\" in block \"" << block.name << "\" was removed since the texture does not exist";
			*it = TEXTURE_NOTFOUND;
		}
	};
	//checkTextures(std::begin(block.textureFaces), std::end(block.textureFaces));
	//checkTextures(block.modelTextures.data(), block.modelTextures.data() + block.modelTextures.size());
}

void workshop::validateItem(Assets* assets, ItemDef& item) {
	const Atlas* const atlas = getAtlas(assets, item.icon);
	if (!atlas->has((item.iconType == ItemIconType::BLOCK ? item.icon : getTexName(item.icon)))) {
		logger.info() << "invalid icon \"" << item.icon << "\" was reset in item \"" << item.name << "\"";
		item.iconType = ItemIconType::SPRITE;
		item.icon = "blocks:notfound";
	}
}

std::pair<std::string, bool> workshop::checkPackId(const std::wstring& id, const std::vector<ContentPack>& scanned) {
	if (id.length() < 2) return { "Id too short", false };
	if (id.length() > 24) return { "Id too long", false };
	if (iswdigit(id[0])) return { "Id must not start with a digit", false };
	if (!std::all_of(id.begin(), id.end(), [](const wchar_t c) { return c < 255 && (iswalnum(c) || c == '_'); }))
		return { "Id has illegal character", false };
	if (std::find(ContentPack::RESERVED_NAMES.begin(), ContentPack::RESERVED_NAMES.end(), util::wstr2str_utf8(id)) != ContentPack::RESERVED_NAMES.end())
		return { "Id is reserved", false };
	if (std::find_if(scanned.begin(), scanned.end(), [id](const ContentPack& pack) {
		return util::wstr2str_utf8(id) == pack.id; }
		) != scanned.end()
	) return { "Pack with id exist", false };
	return { "Id available", true };
}

bool workshop::hasFocusedTextbox(const gui::Container& container) {
	for (const auto& elem : container.getNodes()) {
		if (const gui::TextBox* const textBox = dynamic_cast<gui::TextBox*>(elem.get())) {
			if (textBox->isFocused()) return true;
		}
		else if (const gui::Container* const container = dynamic_cast<gui::Container*>(elem.get())) {
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

std::string workshop::lowerCase(const std::string& string) {
	return util::wstr2str_utf8(util::lower_case(util::str2wstr_utf8(string)));
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

std::string workshop::getPrimitiveName(PrimitiveType type) {
	const char* const names[] = { "AABB", "Tetragon", "Hitbox" };
	if (static_cast<size_t>(type) >= std::size(names)) return std::string();
	return names[static_cast<size_t>(type)];
}
