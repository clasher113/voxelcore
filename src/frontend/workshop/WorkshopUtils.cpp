#include "WorkshopUtils.hpp"

#include "assets/Assets.hpp"
#include "constants.hpp"
#include "content/ContentPack.hpp"
#include "core_defs.hpp"
#include "debug/Logger.hpp"
#include "graphics/core/Atlas.hpp"
#include "graphics/core/Texture.hpp"
#include "items/ItemDef.hpp"
#include "maths/UVRegion.hpp"
#include "voxels/Block.hpp"
#include "../UiDocument.hpp"
#include "IncludeCommons.hpp"

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

const std::vector<std::string> DEFAULT_BLOCK_PROPERTIES{
    "parent", "caption", "texture", "texture-faces", "model", "model-name",
    "model-primitives", "material", "rotation", "hitboxes", "hitbox", "emission",
    "size", "obstacle", "replaceable", "light-passing", "sky-light-passing",
    "shadeless", "ambient-occlusion", "breakable", "selectable", "grounded",
    "hidden", "draw-group", "picking-item", "surface-replacement", "script-name",
    "ui-layout", "inventory-size", "tick-interval", "overlay-texture",
    "translucent", "fields", "particles", "icon-type", "icon", "placing-block", 
    "stack-size", "name", "script-file", "culling", /*"data-struct"*/
};

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

std::vector<glm::vec3> workshop::aabb2tetragons(const glm::vec3& a, const glm::vec3& b) {
	return {
		// west (right)
		{ a.x, a.y, a.z },
		{ a.x, a.y, b.z },
		{ a.x, b.y, b.z },
		{ a.x, b.y, a.z },

		// east (left)
		{ b.x, a.y, b.z },
		{ b.x, a.y, a.z },
		{ b.x, b.y, a.z },
		{ b.x, b.y, b.z },

		// down (bottom)
		{ a.x, a.y, a.z },
		{ b.x, a.y, a.z },
		{ b.x, a.y, b.z },
		{ a.x, a.y, b.z },

		// up (top)
		{ a.x, b.y, b.z },
		{ b.x, b.y, b.z },
		{ b.x, b.y, a.z },
		{ a.x, b.y, a.z },

		// north (front)
		{ b.x, a.y, a.z },
		{ a.x, a.y, a.z },
		{ a.x, b.y, a.z },
		{ b.x, b.y, a.z },

		// south (back)
		{ a.x, a.y, b.z },
		{ b.x, a.y, b.z },
		{ b.x, b.y, b.z },
		{ a.x, b.y, b.z },
	};
}

void workshop::formatTextureImage(gui::Image& image, Texture* const texture, float height, const UVRegion& region) {
	const glm::vec2 textureSize((region.u2 - region.u1) * texture->getWidth(), (region.v2 - region.v1) * texture->getHeight());
	const glm::vec2 multiplier(height / textureSize);
	image.setSize(textureSize * std::min(multiplier.x, multiplier.y));
	image.setPos(glm::vec2(height / 2.f - image.getSize().x / 2.f, height / 2.f - image.getSize().y / 2.f));
	image.setTexture(texture);
	image.setUVRegion(region);
}

template void workshop::setSelectable<gui::IconButton>(const gui::Container&);
template void workshop::setSelectable<gui::Button>(const gui::Container&);

template<typename T>
void workshop::setSelectable(const gui::Container& panel) {
	for (const auto& elem : panel.getNodes()) {
		if (T* node = dynamic_cast<T*>(elem.get())) {
			node->listenAction([node, &panel](gui::GUI* gui) {
				if (node->getParent() == nullptr) return;
				for (const auto& elem : panel.getNodes()) {
					if (typeid(*elem.get()) == typeid(T)) {
						deselect(*elem);
					}
				}
				setSelected(*node);
			});
		}
	}
}

void workshop::setSelected(gui::UINode& node){
	node.setColor(glm::vec4(0.05f, 0.1f, 0.15f, 0.75f));
}

void workshop::deselect(gui::UINode& node) {
	node.setColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.95f));
	node.setHoverColor(glm::vec4(0.05f, 0.1f, 0.15f, 0.75f));
}

std::array<glm::vec3, 4> workshop::exportTetragon(const dv::value& list) {
	glm::vec3 temp[3];
	for (size_t i = 0; i < 3; i++) {
		for (size_t j = 0; j < 3; j++) {
			temp[i][j] = list[i * 3 + j].asNumber();
		}
	}
	return {
		temp[0],
		temp[0] + temp[1],
		temp[0] + temp[1] + temp[2],
		temp[0] + temp[2]
	};
}

AABB workshop::exportAABB(const dv::value& list) {
	glm::vec3 temp[2];
	for (size_t i = 0; i < 2; i++) {
		for (size_t j = 0; j < 3; j++) {
			temp[i][j] = list[i * 3 + j].asNumber();
		}
	}
	return AABB(temp[0], temp[1]);
}

std::vector<dv::value*> workshop::getAllWithType(dv::value& object, dv::value_type type) {
	std::vector<dv::value*> result;
	if (object.getType() == type) {
		result.emplace_back(&object);
	} else if (object.getType() == dv::value_type::list) {
		for (dv::value& elem : object) {
			for (dv::value* subElement : getAllWithType(elem, type)) {
				result.emplace_back(subElement);
			}
		}
	} else if (object.getType() == dv::value_type::object) {
		for (auto& [key, value] : const_cast<dv::objects::Object&>(object.asObject())) {
			for (dv::value* subElement : getAllWithType(value, type)) {
				result.emplace_back(subElement);
			}
		}
	}

	return result;
}

void workshop::putVec3(dv::value& list, const glm::vec3& vector) {
	list.add(vector.x); list.add(vector.y); list.add(vector.z);
}

void workshop::putAABB(dv::value& list, AABB aabb) {
	aabb.b -= aabb.a;
	putVec3(list, aabb.a); putVec3(list, aabb.b);
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
	if (block.customModelRaw.getType() != dv::value_type::object) {
		block.customModelRaw = dv::object();
	}
	block.modelName = block.name + ".model";
	Atlas* blocksAtlas = assets->get<Atlas>("blocks");
	std::vector<dv::value*> textures = getAllWithType(block.customModelRaw, dv::value_type::string);
	auto hasTexture = [&block, blocksAtlas](const std::string& textureName) {
		if (!blocksAtlas->has(textureName)) {
			if (textureName != "") {
				logger.info() << "the texture \"" << textureName << "\" in block \"" << block.name
					<< "\" was removed since the texture does not exist";
			}
			return false;
		}
		return true;
	};
	for (std::string& textureName : block.textureFaces){
		if (hasTexture(textureName)) continue;
		textureName = TEXTURE_NOTFOUND;
	}
	for (dv::value* textureName : textures){
		if (hasTexture(textureName->asString())) continue;
		*textureName = TEXTURE_NOTFOUND;
	}
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

std::wstring workshop::getPrimitiveName(PrimitiveType type) {
	const wchar_t* const names[] = { L"AABB", L"Tetragon", L"Hitbox" };
	if (static_cast<size_t>(type) >= std::size(names)) return std::wstring();
	return names[static_cast<size_t>(type)];
}
