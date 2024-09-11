#include "../WorkshopScreen.hpp"

#include "../../../frontend/UiDocument.hpp"
#include "../IncludeCommons.hpp"

void workshop::WorkShopScreen::createUILayoutList(bool showAll, unsigned int column, float posX,
	const std::function<void(const std::string&)>& callback)
{
	createPanel([this, showAll, callback]() {
		gui::Panel& panel = *new gui::Panel(glm::vec2(200));
		if (!showAll) {
			panel += new gui::Button(L"Create layout", glm::vec4(10.f), [this](gui::GUI*) {
				createDefActionPanel(DefAction::CREATE_NEW, DefType::UI_LAYOUT);
			});
		}

		auto createList = [this, &panel, showAll, callback](const std::string& searchName) {
			std::unordered_multimap<std::string, fs::path> layouts;
			if (showAll) {
				layouts.emplace("", fs::path());
				for (const ContentPack& pack : engine->getContentPacks()) {
					auto files = getFiles(pack.folder / "layouts", false);
					for (const auto& file : files)
						layouts.emplace(pack.id, file);
				}
			}
			else {
				auto files = getFiles(currentPack.folder / "layouts", false);
				for (const auto& file : files)
					layouts.emplace(currentPackId, file);
			}

			for (const auto& elem : layouts) {
				if (elem.second.extension() != ".xml") continue;
				const std::string actualName(elem.second.stem().string());
				if (actualName.find(searchName) == std::string::npos) continue;
				const std::string fileName(elem.first + ':' + actualName);
				const std::string displayName(actualName.empty() ? NOT_SET : showAll ? fileName : actualName);
				if (assets->get<UiDocument>(fileName) || elem.second.string().empty()) {
					gui::Button& button = *new gui::Button(util::str2wstr_utf8(displayName), glm::vec4(10.f), [this, elem, fileName, actualName, callback](gui::GUI*) {
						if (callback) callback(fileName);
						else createUILayoutEditor(elem.second, fileName, actualName, {});
					});
					button.setTextAlign(gui::Align::left);
					panel += removeList.emplace_back(&button);
				}
			}
			setSelectable<gui::Button>(panel);
		};

		gui::TextBox& textBox = *new gui::TextBox(L"Search");
		textBox.setTextValidator([this, &panel, createList, &textBox](const std::wstring&) {
			clearRemoveList(panel);
			createList(util::wstr2str_utf8(textBox.getInput()));
			return true;
		});
		panel += textBox;

		createList("");

		return std::ref(panel);
	}, column, posX);
}