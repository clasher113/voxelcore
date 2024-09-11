#include "../WorkshopScreen.hpp"

#include "../../../assets/Assets.hpp"
#include "../../../constants.hpp"
#include "../../../graphics/core/Atlas.hpp"
#include "../IncludeCommons.hpp"

void workshop::WorkShopScreen::createTextureList(float icoSize, unsigned int column, DefType type, float posX, bool showAll,
	const std::function<void(const std::string&)>& callback)
{
	createPanel([this, showAll, callback, posX, type]() {
		gui::Panel& panel = *new gui::Panel(glm::vec2(settings.textureListWidth));
		panel.setScrollable(true);
		optimizeContainer(panel);

		auto createList = [this, &panel, showAll, callback, posX, type](const std::string& searchName) {
			auto& packs = engine->getContentPacks();
			std::unordered_map<std::string, fs::path> paths;
			paths.emplace(currentPack.title, currentPack.folder);
			if (showAll) {
				for (const auto& elem : packs) {
					paths[elem.title] = elem.folder;
				}
				paths.emplace("Default", engine->getPaths()->getResources());
			}

			for (const auto& elem : paths) {
				std::unordered_map<fs::path, DefType> defPaths;
				if (type == DefType::BLOCK || type == DefType::BOTH) defPaths.emplace(elem.second / TEXTURES_FOLDER / ContentPack::BLOCKS_FOLDER, DefType::BLOCK);
				if (type == DefType::ITEM || type == DefType::BOTH) defPaths.emplace(elem.second / TEXTURES_FOLDER / ContentPack::ITEMS_FOLDER, DefType::ITEM);
				bool addPackName = true;
				for (const auto& defPath : defPaths) {
					if (!fs::exists(defPath.first)) continue;
					const Atlas* atlas = assets->get<Atlas>(getDefFolder(defPath.second));
					std::vector<fs::path> files = getFiles(defPath.first, false);
					if (!searchName.empty()) {
						std::sort(files.begin(), files.end(), [](const fs::path& a, const fs::path& b) {
							return a.stem().string().size() < b.stem().string().size();
						});
					}
					for (const auto& entry : files) {
						const std::string file = entry.stem().string();
						if (!searchName.empty()) {
							if (file.find(searchName) == std::string::npos) continue;
						}
						if (addPackName && showAll) {
							panel += removeList.emplace_back(new gui::Label(elem.first));
							addPackName = false;
						}
						if (!atlas->has(file)) continue;
						gui::IconButton& button = *new gui::IconButton(glm::vec2(panel.getSize().x, 50.f), file, atlas, file);
						button.listenAction([this, file, defPath, callback](gui::GUI*) {
							if (callback) callback(getDefFolder(defPath.second) + ':' + file);
							else createTextureInfoPanel(getDefFolder(defPath.second) + ':' + file, defPath.second);
						});
						panel += removeList.emplace_back(&button);
					}
				}
			}
			setSelectable<gui::IconButton>(panel);
		};
		if (!showAll) {
			panel += new gui::Button(L"Import", glm::vec4(10.f), [this](gui::GUI*) {
				createImportPanel();
			});
		}
		gui::TextBox& textBox = *new gui::TextBox(L"Search");
		textBox.setTextValidator([this, &panel, createList, &textBox](const std::wstring&) {
			clearRemoveList(panel);
			createList(util::wstr2str_utf8(textBox.getInput()));
			return true;
		});

		panel += textBox;

		createList(util::wstr2str_utf8(textBox.getInput()));

		return std::ref(panel);
	}, column, posX);
}