#include "menu_workshop.hpp"

#include "coders/png.hpp"
#include "constants.hpp"
#include "engine/Engine.hpp"
#include "graphics/core/Texture.hpp"
#include "graphics/ui/elements/Container.hpp"
#include "graphics/ui/elements/Menu.hpp"
#include "graphics/ui/GUI.hpp"
#include "graphics/ui/gui_util.hpp"
#include "util/stringutil.hpp"
#include "IncludeCommons.hpp"
#include "WorkshopScreen.hpp"
#include "WorkshopSerializer.hpp"
#include "files/files.hpp"
#include "graphics/ui/gui_xml.hpp"
#include "assets/AssetsLoader.hpp"

struct FilterSettings {
	uint8_t reverseSort = false;
	uint8_t sortType = 0;
	uint8_t workingDirectory = 0;
	std::string searchString;
	std::string worldPath;
};

static std::vector<std::string> readHistory(const fs::path& configFolder){
	fs::path historyFile(configFolder / HISTORY_FILE);
	if (fs::is_regular_file(historyFile)) {
		std::vector<ubyte> bytes = files::read_bytes(historyFile);
		return util::split(std::string(reinterpret_cast<char*>(bytes.data()), bytes.size()), '\0');
	}
	return {};
}

static void writeHistory(const fs::path& configFolder, std::vector<std::string> history, const std::string& newItem) {
	auto found = std::find(history.begin(), history.end(), newItem);
	if (found != history.end()) history.erase(found);
	history.emplace(history.begin(), newItem);
	std::vector<char> byteArr;
	for (const std::string& elem : history){
		byteArr.insert(byteArr.end(), elem.begin(), elem.end());
		byteArr.emplace_back('\0');
	}
	files::write_bytes(configFolder / HISTORY_FILE, reinterpret_cast<ubyte*>(byteArr.data()), byteArr.size());
}

static FilterSettings readFilter(const fs::path& configFolder) {
	FilterSettings filter;
	fs::path filterFile(configFolder / FILTER_FILE);
	if (fs::is_regular_file(filterFile)) {
		const size_t offset = offsetof(FilterSettings, searchString);
		dv::value root = files::read_binary_json(filterFile);
		dv::objects::Bytes& bytes = root["bytes"].asBytes();
		memcpy(&filter, bytes.data(), std::min(bytes.size(), offset));
		filter.worldPath = root["world-path"].asString();
		filter.searchString = root["search-string"].asString();
	}
	return filter;
}

static void writeFilter(const fs::path& configFolder, FilterSettings filter){
	const size_t offset = offsetof(FilterSettings, searchString);
	dv::value root = dv::object();
	dv::objects::Bytes bytes(offset);
	memcpy(bytes.data(), &filter, offset);
	root["bytes"] = bytes;
	root["world-path"] = filter.worldPath;
	root["search-string"] = filter.searchString;
	files::write_binary_json(configFolder / FILTER_FILE, root);
}

void workshop::create_workshop_button(Engine& engine, const gui::Page* page) {
	if (page->name != "main") return;

	auto menu = engine.getGUI()->getMenu();
	gui::Button& button = *new gui::Button(L"Workshop", glm::vec4(10.f), [menu](gui::GUI*) {
		menu->setPage("workshop");
	});

	std::shared_ptr<gui::Panel> panel = std::dynamic_pointer_cast<gui::Panel>(page->panel);
	if (!panel) return;
	std::vector<std::shared_ptr<gui::UINode>> nodes = panel->getNodes();
	panel->clear();
	for (size_t i = 0; i < nodes.size(); i++) {
		if (i == nodes.size() - 1) *panel << button;
		panel->add(nodes[i]);
	}
	menu->setSize(panel->getSize());

	panel = std::make_shared<gui::Panel>(glm::vec2(800, 600), glm::vec4(10.f), 10.f);
	panel->setOrientation(gui::Orientation::horizontal);
	panel->setColor(glm::vec4(0.5f, 0.5f, 0.5f, 0.5f));
	gui::Panel& filterPanel = *new gui::Panel(glm::vec2(385, 580));
	filterPanel.setMinLength(filterPanel.getSize().y);
	filterPanel.setMaxLength(filterPanel.getSize().y);
	gui::Panel& packsPanel = *new gui::Panel(glm::vec2(385, 580));
	packsPanel.setMinLength(packsPanel.getSize().y);
	packsPanel.setMaxLength(packsPanel.getSize().y);
	packsPanel.setId("packs");
	filterPanel.setId("filter");
	*panel << filterPanel << packsPanel;

	const fs::path configFolder(workshop::getConfigFolder(engine));
	if (!fs::is_directory(configFolder)) fs::create_directories(configFolder);

	menu->addPage("workshop", std::shared_ptr<gui::Panel>(panel));
	std::vector<std::string> openHistory = readHistory(configFolder);
	static FilterSettings filter = readFilter(configFolder);
	const std::wstring sortTypes[] = { L"Id (A-Z)", L"Title (A-Z)", L"Last modify", L"Last open" };
	const std::wstring directories[] = { L"/res/content/", L"/content/" };
	auto getWorkingDirectoryName = [directories]() {
		if (filter.workingDirectory >= std::size(directories)) {
			return util::str2wstr_utf8(filter.worldPath);
		}
		return directories[filter.workingDirectory];
	};

	gui::Button& sortButton = *new gui::Button(L"Sort by: " + sortTypes[filter.sortType], glm::vec4(10.f), gui::onaction());
	gui::FullCheckBox& sortCheckBox = *new gui::FullCheckBox(L"Reverse sort", glm::vec2(filterPanel.getSize().x, 30.f), filter.reverseSort);
	gui::TextBox& searchBox = *new gui::TextBox(L"");
	searchBox.setHint(L"Search by id/title");
	searchBox.setText(util::str2wstr_utf8(filter.searchString));
	gui::Button& directoryButton = *new gui::Button(L"Working directory: " + getWorkingDirectoryName(), glm::vec4(10.f), gui::onaction());
	gui::Panel& worldsPanel = *new gui::Panel(glm::vec2(filterPanel.getSize().x, 320.f));
	worldsPanel.setMaxLength(worldsPanel.getSize().y);
	worldsPanel.setMinLength(worldsPanel.getSize().y);
	filterPanel << sortButton << sortCheckBox << searchBox << directoryButton;
	filterPanel << new gui::Label(L"Worlds with local packs:");
	filterPanel << new gui::Label(L"(Click to show world packs)");
	filterPanel << worldsPanel;

	std::unordered_map<fs::path, std::vector<ContentPack>> packs;
	
	auto fetchPacks = [&packs](const std::string& path, const fs::path& folder, const std::string& key) {
		std::vector<ContentPack> userPacks;
		ContentPack::scanFolder(path, folder, userPacks);
		if (!userPacks.empty()) {
			packs.emplace(key, userPacks);
		}
	};

	auto worlds = engine.getPaths().scanForWorlds();
	for (const fs::path& path : worlds) {
		fetchPacks("world:content", path / "content", path.string());
	}
	fetchPacks("res:content", "res/content", util::wstr2str_utf8(directories[0]));
	fetchPacks("user:content", "content", util::wstr2str_utf8(directories[1]));

	auto getPacks = [=]() -> std::vector<ContentPack> {
		fs::path key(getWorkingDirectoryName());
		if (packs.find(key) != packs.end()){
			return packs.at(key);
		}
		return {};
	};

	auto createPackList = [=, &packsPanel, &engine]() {
		std::vector<ContentPack> scanned = getPacks();
		packsPanel.clear();
		if (scanned.empty()){
			gui::Label& label = *new gui::Label("Empty");
			label.setSize(packsPanel.getSize() - 10.f);
			label.setAlign(gui::Align::center);
			packsPanel << label;
		}
		std::sort(scanned.begin(), scanned.end(), [openHistory](const ContentPack& a, const ContentPack& b) {
			switch (filter.sortType) {
				case 0: return lowerCase(a.id) < lowerCase(b.id);
				case 1: return lowerCase(a.title) < lowerCase(b.title);
				case 2: return fs::last_write_time(a.folder) > fs::last_write_time(b.folder);
				case 3: return std::distance(openHistory.begin(), std::find(openHistory.begin(), openHistory.end(), a.id)) < 
					std::distance(openHistory.begin(), std::find(openHistory.begin(), openHistory.end(), b.id));
			}
			return false;
		});
		if (filter.reverseSort) {
			std::reverse(scanned.begin(), scanned.end());
		}
		if (!filter.searchString.empty()){
			std::string searchLower = lowerCase(filter.searchString);
			scanned.erase(std::remove_if(scanned.begin(), scanned.end(), [searchLower](const ContentPack& pack) {
				return lowerCase(pack.id).find(searchLower) == std::string::npos && lowerCase(pack.title).find(searchLower) == std::string::npos;
			}), scanned.end());
		}
		for (const auto& pack : scanned) {
			gui::Container& container = *new gui::Container(glm::vec2(panel->getSize().x, 80.f));
			container.setColor(glm::vec4(0.f, 0.f, 0.f, 0.25f));
			container.setHoverColor(glm::vec4(0.f, 0.f, 0.f, 0.5f));
			container.listenAction([&engine, pack, openHistory, configFolder](gui::GUI*) {
				writeFilter(configFolder, filter);
				writeHistory(configFolder, openHistory, pack.id);
				engine.setScreen(std::make_shared<workshop::WorkShopScreen>(engine, pack));
			});

			auto getIcon = [&engine, pack]()->std::string {
				std::string icon = pack.id + ".icon";
				Assets* assets = engine.getAssets();
				const Texture* const iconTex = assets->get<Texture>(icon);
				if (iconTex == nullptr) {
					auto iconfile = pack.folder / fs::path("icon.png");
					if (fs::is_regular_file(iconfile)) {
						assets->store(png::load_texture(iconfile.string()), icon);
					}
					else return "gui/no_icon";
				}
				return icon;
			};

			container.add(std::make_shared<gui::Image>(getIcon(), glm::vec2(64.f)), glm::vec2(8.f));
			container.add(std::make_shared<gui::Label>(pack.title), glm::vec2(78.f, 6.f));
			auto label = std::make_shared<gui::Label>(pack.description);
			label->setColor(glm::vec4(1.f, 1.f, 1.f, 0.7f));
			container.add(label, glm::vec2(80.f, 28.f));

			packsPanel << container;
		}
	};
	sortButton.listenAction([=, &sortButton](gui::GUI*) {
		filter.sortType++;
		if (filter.sortType >= std::size(sortTypes)) filter.sortType = 0;
		sortButton.setText(L"Sort by: " + sortTypes[filter.sortType]);
		createPackList();
	});
	directoryButton.listenAction([=, &engine, &directoryButton, &worldsPanel](gui::GUI*) mutable {
		filter.workingDirectory++;
		if (filter.workingDirectory >= std::size(directories)) filter.workingDirectory = 0;
		directoryButton.setText(L"Working directory: " + getWorkingDirectoryName());
		createPackList();
	});
	sortCheckBox.setConsumer([createPackList, &searchBox](bool checked) {
		filter.reverseSort = checked;
		createPackList();
	});
	searchBox.setTextValidator([createPackList](const std::wstring& string) {
		filter.searchString = util::wstr2str_utf8(string);
		createPackList();
		return true;
	});

	for (const fs::path& path : worlds) {
		if (packs.find(path) != packs.end()) {
			dv::value root = files::read_json(path / fs::u8path("world.json"));

			gui::Container& worldContainer = *new gui::Container(glm::vec2(380.f, 66.f));
			worldContainer.listenAction([=, &directoryButton](gui::GUI*) {
				filter.worldPath = path.string();
				filter.workingDirectory = std::numeric_limits<uint8_t>::max();
				directoryButton.setText(L"Working directory: " + getWorkingDirectoryName());
				createPackList();
			});
			worldContainer.setColor(glm::vec4(0.06f, 0.11f, 0.17f, 0.7f));
			worldContainer.setHoverColor(glm::vec4(0.08f, 0.17f, 0.2f, 0.6f));

			std::string icon = "world#" + root["name"].asString() + ".icon";
			if (!AssetsLoader::loadExternalTexture(engine.getAssets(), icon, { path / fs::path("preview.png") }))
				icon = "gui/no_world_icon";

			worldContainer << new gui::Image(icon, glm::vec2(96.f, 64.f));
			gui::Label& worldName = *new gui::Label(root["name"].asString());
			worldName.setPos(glm::vec2(104.f, 25.f));
			worldContainer << worldName;
			worldsPanel << worldContainer;
		}
	}
	if (worlds.empty()) {
		gui::Label& label = *new gui::Label("Empty");
		label.setSize(worldsPanel.getSize() - 10.f);
		label.setAlign(gui::Align::center);
		worldsPanel << label;
	}

	createPackList();

	filterPanel << new gui::Button(L"Create new", glm::vec4(10.f), [=, &engine](gui::GUI*) {
		auto panel = std::make_shared<gui::Panel>(glm::vec2(400, 200));
		menu->addPage("new-content", panel);
		menu->setPage("new-content");
		fs::path path = engine.getPaths().getResourcesFolder() / "content";

		*panel << new gui::Label(L"Name");
		gui::TextBox& nameInput = *new gui::TextBox(L"example_pack", glm::vec4(6.0f));
		nameInput.setTextValidator([getPacks](const std::wstring& text) {
			return checkPackId(text, getPacks());
		});
		*panel << nameInput;

		*panel << new gui::Button(L"Create", glm::vec4(10.f), [=, &nameInput, &engine](gui::GUI*) {
			if (!nameInput.validate()) return;
			ContentPack pack;
			pack.folder = path / nameInput.getInput();
			pack.id = pack.title = util::wstr2str_utf8(nameInput.getInput());
			pack.version = "1.0";
			fs::create_directories(pack.folder / ContentPack::BLOCKS_FOLDER);
			fs::create_directories(pack.folder / ContentPack::ITEMS_FOLDER);
			fs::create_directories(pack.folder / TEXTURES_FOLDER);
			saveContentPack(pack);
			menu->remove(panel);

			writeFilter(configFolder, filter);
			writeHistory(configFolder, openHistory, pack.id);
			engine.setScreen(std::make_shared<workshop::WorkShopScreen>(engine, pack));
		});
		*panel << new gui::Button(L"Back", glm::vec4(10.f), [menu](gui::GUI*) {
			menu->back();
		});
	});
	filterPanel << new gui::Button(L"Back", glm::vec4(10.f), [menu](gui::GUI*) {
		menu->back();
	});
}