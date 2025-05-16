#include "frontend/workshop/WorkshopScreen.hpp"

#include "assets/Assets.hpp"
#include "content/Content.hpp"
#include "graphics/core/Atlas.hpp"
#include "items/ItemDef.hpp"
#include "graphics/ui/elements/Panel.hpp"
#include "graphics/ui/elements/Button.hpp"
#include "graphics/ui/elements/Textbox.hpp"
#include "frontend/workshop/gui_elements/IconButton.hpp"
#include "frontend/workshop/WorkshopPreview.hpp"
#include "objects/EntityDef.hpp"
#include "frontend/UiDocument.hpp"
#include "graphics/commons/Model.hpp"
#include "graphics/core/Texture.hpp"
#include "engine/Engine.hpp"

#include <algorithm>

using namespace workshop;

struct ListButtonInfo{
	std::string fullName;
	std::string displayName;
	UVRegion region;
	Texture* texture;
	static void sortAlphabetically(std::vector<ListButtonInfo>& list){
		std::sort(list.begin(), list.end(), [](const ListButtonInfo& a, const ListButtonInfo& b) {
			return a.displayName < b.displayName;
		});
	}
};

static void createList(const std::vector<ListButtonInfo> list, bool icons, gui::Panel& dstPanel, const std::function<void(const std::string&)>& callback = 0,
	float iconSize = 25.f)
{
	auto createList = [=, &dstPanel](const std::string& searchName) {
		std::vector<ListButtonInfo> list_copy = list;
		if (!searchName.empty()){
			list_copy.erase(std::remove_if(list_copy.begin(), list_copy.end(), [searchName](const ListButtonInfo& info) {
				return info.displayName.find(searchName) == std::string::npos;
			}), list_copy.end());
			std::sort(list_copy.begin(), list_copy.end(), [](const ListButtonInfo& a, const ListButtonInfo& b) {
				return a.displayName.size() < b.displayName.size();
			});
		}
		for (const ListButtonInfo& info : list_copy) {
			gui::UINode* node = nullptr;
			if (icons) {
				node = new gui::IconButton(glm::vec2(dstPanel.getSize().x, iconSize), info.displayName, info.texture, info.region);
			}
			else {
				gui::Button& button = *new gui::Button(util::str2wstr_utf8(info.displayName), glm::vec4(10.f), gui::onaction());
				button.setTextAlign(gui::Align::left);
				node = &button;
			}
			node->listenAction([=](gui::GUI*) {
				if (callback) callback(info.fullName);
			});
			dstPanel << markRemovable(node);
		}
		if (icons) setSelectable<gui::IconButton>(dstPanel);
		else setSelectable<gui::Button>(dstPanel);
	};

	gui::TextBox& textBox = *new gui::TextBox(L"Search");
	textBox.setTextValidator([&dstPanel, createList, &textBox](const std::wstring&) {
		removeRemovable(dstPanel);
		createList(util::wstr2str_utf8(textBox.getInput()));
		return true;
	});
	dstPanel << textBox;

	optimizeContainer(dstPanel);

	createList("");
}

void workshop::WorkShopScreen::createContentList(ContentType type, bool showAll, unsigned int column, float posX, 
	const std::function<void(const std::string&)>& callback)
{
	createPanel([this, type, showAll, callback]() {
		gui::Panel& panel = *new gui::Panel(glm::vec2(settings.contentListWidth));

		if (!showAll) {
			panel << new gui::Button(L"Create " + util::str2wstr_utf8(getDefName(type)), glm::vec4(10.f), [this, type](gui::GUI*) {
				createDefActionPanel(ContentAction::CREATE_NEW, type);
			});
		}

		std::vector<ListButtonInfo> list;

		const size_t size = (type == ContentType::BLOCK ? indices->blocks.count() : indices->items.count());
		std::vector<std::pair<std::string, std::string>> sorted;
		for (size_t i = 0; i < size; i++) {
			std::string fullName;
			if (type == ContentType::ITEM) fullName = indices->items.get(static_cast<itemid_t>(i))->name;
			else if (type == ContentType::BLOCK) fullName = indices->blocks.get(static_cast<blockid_t>(i))->name;
			if (!showAll && fullName.find(currentPackId) != 0) continue;
			sorted.emplace_back(fullName, getDefName(fullName));
		}
		std::sort(sorted.begin(), sorted.end(), [this, type, showAll](const decltype(sorted[0])& a, const decltype(sorted[0])& b) {
			if (type == ContentType::ITEM) {
				auto hasFile = [this](const std::string& name) {
					return fs::is_regular_file(currentPack.folder / ContentPack::ITEMS_FOLDER / std::string(name + ".json"));
				};
				return hasFile(a.second) > hasFile(b.second) || (hasFile(a.second) == hasFile(b.second) &&
					(showAll ? a.first : a.second) < (showAll ? b.first : b.second));
			}
			if (showAll) return a.first < b.first;
			return a.second < b.second;
		});

		for (const auto& [fullName, actualName] : sorted) {
			Atlas* contentAtlas = previewAtlas;
			std::string textureName(fullName);

			if (type == ContentType::ITEM) {
				ItemDef& item = *const_cast<ItemDef*>(content->items.find(fullName));
				validateItem(assets, item);
				if (item.iconType == ItemIconType::BLOCK)
					textureName = item.icon;
				else {
					contentAtlas = getAtlas(assets, item.icon);
					textureName = getTexName(item.icon);
				}
			}

			list.emplace_back(ListButtonInfo{ fullName, showAll ? fullName : actualName, contentAtlas->get(textureName), contentAtlas->getTexture() });
		}

		createList(list, true, panel, [this, type, callback](const std::string& string) {
			if (callback) callback(string);
			else {
				if (type == ContentType::BLOCK) 
					createBlockEditor(*const_cast<Block*>(content->blocks.find(string)));
				else if (type == ContentType::ITEM)
					createItemEditor(*const_cast<ItemDef*>(content->items.find(string)));
			}
		}, 32.f);

		return std::ref(panel);
	}, column, posX);
}

void WorkShopScreen::createTextureList(float iconSize, unsigned int column, std::vector<ContentType> types, float posX, bool showAll,
	const std::function<void(const std::string&)>& callback)
{
	createPanel([this, showAll, callback, types, iconSize]() {
		gui::Panel& panel = *new gui::Panel(glm::vec2(settings.textureListWidth));

		std::vector<ListButtonInfo> list;
		std::unordered_map<std::string, fs::path> paths;

		for (const auto& pack : engine.getContentPacks()) {
			if (!showAll && pack.id != currentPackId) continue;
			paths[pack.title] = pack.folder;
		}
		if (showAll) paths.emplace("Default", engine.getPaths().getResourcesFolder());

		for (const auto& [packId, packPath] : paths) {
			for (const auto& type : types) {
				fs::path path(packPath / TEXTURES_FOLDER / getDefFolder(type));
				if (!fs::exists(path)) continue;
				Texture* texture = nullptr;
				UVRegion uv;

				for (const auto& entry : getFiles(path, false)) {
					std::string file = entry.stem().string();
					if (type == ContentType::ENTITY)
						texture = assets->get<Texture>(getDefFolder(type) + '/' + file);
					else {
						Atlas* atlas = assets->get<Atlas>(getDefFolder(type));
						texture = atlas->getTexture();
						uv = atlas->get(file);
					}
					if (texture)
						list.emplace_back(ListButtonInfo{std::to_string(static_cast<int>(type)) + ';' + file, file, uv, texture});
				}
			}
		}
		if (!showAll) {
			panel << new gui::Button(L"Import", glm::vec4(10.f), [this](gui::GUI*) {
				createImportPanel();
			});
		}

		createList(list, true, panel, [this, callback](const std::string& string){
			std::vector<std::string> split = util::split(string, ';');
			const ContentType type = static_cast<ContentType>(stoi(split[0]));
			if (callback) callback(getDefFolder(type) + ':' + split[1]);
			else {
				if (type == ContentType::ENTITY)
					createTextureInfoPanel(getDefFolder(type) + '/' + split[1], type);
				else
					createTextureInfoPanel(getDefFolder(type) + ':' + split[1], type);
			}
		}, iconSize);

		return std::ref(panel);
	}, column, posX);
}

void WorkShopScreen::createMaterialsList(bool showAll, unsigned int column, float posX, const std::function<void(const std::string&)>& callback) {
	createPanel([this, showAll, callback]() {
		gui::Panel& panel = *new gui::Panel(glm::vec2(settings.contentListWidth));

		std::vector<ListButtonInfo> list;
		for (auto& elem : content->getBlockMaterials()) {
			if (!showAll && elem.first.find(currentPackId) != 0) continue;
			list.emplace_back(ListButtonInfo{ elem.first, (showAll ? elem.first : getDefName(elem.first)), UVRegion(), 0 });
		}

		ListButtonInfo::sortAlphabetically(list);
		createList(list, false, panel, [this, callback](const std::string& string) {
			if (callback) callback(string);
			else createMaterialEditor(const_cast<BlockMaterial&>(*content->findBlockMaterial(string)));
		});

		return std::ref(panel);
	}, column, posX);
}

void WorkShopScreen::createScriptList(unsigned int column, float posX, const std::function<void(const std::string&)>& callback) {
	createPanel([this, callback]() {
		gui::Panel& panel = *new gui::Panel(glm::vec2(settings.contentListWidth));

		fs::path scriptsFolder(currentPack.folder / "scripts/");
		std::vector<fs::path> scripts = getFiles(scriptsFolder, true);
		if (callback) scripts.emplace(scripts.begin(), fs::path());
		std::vector<ListButtonInfo> list;

		for (const auto& elem : scripts) {
			fs::path parentDir(fs::relative(elem.parent_path(), scriptsFolder));
			std::string fullScriptName = parentDir.string() + "/" + elem.stem().string();
			std::string scriptName = fs::path(getScriptName(currentPack, fullScriptName)).stem().string();

			list.emplace_back(ListButtonInfo{ elem.string() + ';' + fullScriptName, scriptName, UVRegion(), 0 });
		}

		ListButtonInfo::sortAlphabetically(list);
		createList(list, false, panel, [this, callback](const std::string& string) {
			std::vector<std::string> split = util::split(string, ';');
			if (callback) callback(split[1]);
			else createScriptInfoPanel(split[0]);
		});

		return std::ref(panel);
	}, column, posX);
}

void WorkShopScreen::createSoundList() {
	createPanel([this]() {
		gui::Panel& panel = *new gui::Panel(glm::vec2(settings.contentListWidth));

		std::vector<fs::path> sounds = getFiles(currentPack.folder / SOUNDS_FOLDER, true);
		std::vector<ListButtonInfo> list;
		for (const auto& elem : sounds) {
			list.emplace_back(ListButtonInfo{ elem.string(), elem.stem().string(), UVRegion(), 0 });
		}

		ListButtonInfo::sortAlphabetically(list);
		createList(list, false, panel, [this](const std::string& string) {
			createSoundInfoPanel(string);
		});

		return std::ref(panel);
	}, 1);
}

void WorkShopScreen::createUILayoutList(bool showAll, unsigned int column, float posX, const std::function<void(const std::string&)>& callback) {
	createPanel([this, showAll, callback]() {
		gui::Panel& panel = *new gui::Panel(glm::vec2(settings.contentListWidth));
		if (!showAll) {
			panel << new gui::Button(L"Create layout", glm::vec4(10.f), [this](gui::GUI*) {
				createDefActionPanel(ContentAction::CREATE_NEW, ContentType::UI_LAYOUT);
			});
		}

		std::vector<ListButtonInfo> list;

		if (showAll) list.emplace_back(ListButtonInfo{ " ; ", NOT_SET, UVRegion(), 0 });
		for (const ContentPack& pack : engine.getContentPacks()) {
			if (showAll == false && pack.id != currentPackId) continue;
			auto files = getFiles(pack.folder / LAYOUTS_FOLDER, false);
			for (const auto& file : files) {
				if (file.extension() != getDefFileFormat(ContentType::UI_LAYOUT)) continue;
				const std::string actualName(file.stem().string());
				const std::string fullName(pack.id + ':' + actualName);
				const std::string displayName(actualName.empty() ? NOT_SET : showAll ? fullName : actualName);

				list.emplace_back(ListButtonInfo{ file.string() + ";" + fullName, displayName, UVRegion(), 0 });
			}
		}

		ListButtonInfo::sortAlphabetically(list);
		createList(list, false, panel, [this, callback](const std::string& string) {
			std::vector<std::string> split = util::split(string, ';');
			if (callback) callback(split[1]);
			else createUILayoutEditor(split[0], split[1], {});
		});

		return std::ref(panel);
	}, column, posX);
}
void workshop::WorkShopScreen::createEntitiesList(bool showAll, unsigned int column, float posX, 
	const std::function<void(const std::string&)>& callback)
{
	createPanel([this, showAll, callback]() {
		gui::Panel& panel = *new gui::Panel(glm::vec2(settings.contentListWidth));
		if (!showAll) {
			panel << new gui::Button(L"Create " + util::str2wstr_utf8(getDefName(ContentType::ENTITY)), glm::vec4(10.f), [this](gui::GUI*) {
				createDefActionPanel(ContentAction::CREATE_NEW, ContentType::ENTITY);
			});
		}
		std::vector<ListButtonInfo> list;
		if (showAll) list.emplace_back(ListButtonInfo{ "", NOT_SET, UVRegion(), nullptr });
		for (EntityDef* entity : indices->entities.getIterable()) {
			if (!showAll && entity->name.find(currentPackId) != 0) continue;
			list.emplace_back(ListButtonInfo{ entity->name, showAll ? entity->name : getDefName(entity->name), UVRegion(), nullptr });
		}

		ListButtonInfo::sortAlphabetically(list);
		createList(list, false, panel, [this, callback](const std::string& string) {
			if (callback) callback(string);
			else createEntityEditorPanel(*const_cast<EntityDef*>(content->entities.find(string)));
		});

		return std::ref(panel);
	}, column, posX);
}

void workshop::WorkShopScreen::createSkeletonList(bool showAll, unsigned int column, float posX, 
	const std::function<void(const std::string&)>& callback)
{
	createPanel([this, showAll, callback]() {
		gui::Panel& panel = *new gui::Panel(glm::vec2(settings.contentListWidth));
		if (!showAll) {
			panel << new gui::Button(L"Create " + util::str2wstr_utf8(getDefName(ContentType::SKELETON)), glm::vec4(10.f), [this](gui::GUI*) {
				createDefActionPanel(ContentAction::CREATE_NEW, ContentType::SKELETON);
			});
		}

		std::vector<ListButtonInfo> list;
		if (showAll) list.emplace_back(ListButtonInfo{ "", NOT_SET, UVRegion(), nullptr });

		for (auto& skeleton : content->getSkeletons()){
			if (!showAll && skeleton.first.find(currentPackId) != 0) continue;
			list.emplace_back(ListButtonInfo{ skeleton.first, showAll ? skeleton.first : getDefName(skeleton.first), UVRegion(), nullptr });
		}

		ListButtonInfo::sortAlphabetically(list);
		createList(list, false, panel, [this, callback](const std::string& string) {
			if (callback) callback(string);
			else createSkeletonEditorPanel(*const_cast<rigging::SkeletonConfig*>(content->getSkeleton(string)), {});
		});

		return std::ref(panel);
	}, column, posX);
}

void workshop::WorkShopScreen::createModelsList(bool showAll, unsigned int column, float posX, 
	const std::function<void(const std::string&)>& callback)
{
	createPanel([this, showAll, callback]() {
		gui::Panel& panel = *new gui::Panel(glm::vec2(settings.contentListWidth));

		std::vector<ListButtonInfo> list;
		if (showAll) list.emplace_back(ListButtonInfo{ "", NOT_SET, UVRegion(), nullptr });

		for (const ContentPack& pack : engine.getContentPacks()) {
			if (showAll == false && pack.id != currentPackId) continue;
			for (const fs::path& file : getFiles(pack.folder / MODELS_FOLDER, false)) {
				const std::string name = file.stem().string();
				list.emplace_back(ListButtonInfo{ name, name, UVRegion(), nullptr });
			}
		}

		if (!showAll) {
			panel << new gui::Button(L"Import", glm::vec4(10.f), [this](gui::GUI*) {
				createImportPanel(ContentType::MODEL);
			});
		}

		ListButtonInfo::sortAlphabetically(list);
		list.erase(std::unique(list.begin(), list.end(), [](const ListButtonInfo& a, const ListButtonInfo& b) {return a.displayName == b.displayName; }), list.end());
		createList(list, false, panel, [this, callback](const std::string& string) {
			if (callback) callback(string);
			else {
				preview->setModel(assets->get<model::Model>(string));
				createModelPreview(2);
			}
		});

		return std::ref(panel);
	}, column, posX);
}