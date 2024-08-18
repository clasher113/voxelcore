#include "../WorkshopScreen.hpp"

#include "../../../assets/Assets.hpp"
#include "../../../graphics/core/Atlas.hpp"
#include "../IncludeCommons.hpp"
#include "../../../constants.hpp"

void workshop::WorkShopScreen::createTextureList(float icoSize, unsigned int column, DefType type, float posX, bool showAll,
	const std::function<void(const std::string&)>& callback)
{
	createPanel([this, showAll, callback, posX, type]() {
		auto panel = std::make_shared<gui::Panel>(glm::vec2(settings.textureListWidth));
		panel->setScrollable(true);

		auto createList = [this, panel, showAll, callback, posX, type](const std::string& searchName) {
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
				if (showAll) panel->add(removeList.emplace_back(std::make_shared<gui::Label>(elem.first)));
				for (const auto& defPath : defPaths) {
					if (!fs::exists(defPath.first)) continue;
					Atlas* atlas = assets->get<Atlas>(getDefFolder(defPath.second));
					std::vector<fs::path> files = getFiles(defPath.first, false);
					if (!searchName.empty()) {
						std::sort(files.begin(), files.end(), [](const fs::path& a, const fs::path& b) {
							return a.stem().string().size() < b.stem().string().size();
						});
					}
					for (const auto& entry : files) {
						std::string file = entry.stem().string();
						if (!searchName.empty()) {
							if (file.find(searchName) == std::string::npos) continue;
						}
						if (!atlas->has(file)) continue;
						auto button = std::make_shared<gui::IconButton>(glm::vec2(panel->getSize().x, 50.f), file, atlas, file);
						button->listenAction([this, panel, file, defPath, callback, posX](gui::GUI*) {
							if (callback) callback(getDefFolder(defPath.second) + ':' + file);
							else createTextureInfoPanel(getDefFolder(defPath.second) + ':' + file, defPath.second);
						});
						panel->add(removeList.emplace_back(button));
					}
				}
			}
			setSelectable<gui::IconButton>(*panel);
		};
		if (!showAll) {
			panel->add(std::make_shared<gui::Button>(L"Import", glm::vec4(10.f), [this](gui::GUI*) {
				createImportPanel();
			}));
		}
		auto textBox = std::make_shared<gui::TextBox>(L"Search");
		textBox->setTextValidator([this, panel, createList, textBox](const std::wstring&) {
			clearRemoveList(panel);
			createList(util::wstr2str_utf8(textBox->getInput()));
			return true;
		});

		panel->add(textBox);

		createList(util::wstr2str_utf8(textBox->getInput()));

		return panel;
	}, column, posX);
}