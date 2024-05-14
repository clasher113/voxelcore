#include "../WorkshopScreen.hpp"

#include "../IncludeCommons.hpp"

void workshop::WorkShopScreen::createUILayoutList(bool showAll, unsigned int column, float posX,
	const std::function<void(const std::string&)>& callback)
{
	createPanel([this, showAll, callback]() {
		auto panel = std::make_shared<gui::Panel>(glm::vec2(200));
		panel->add(std::make_shared<gui::Button>(L"Create layout", glm::vec4(10.f), [this](gui::GUI*) {
			createDefActionPanel(DefAction::CREATE_NEW, DefType::UI_LAYOUT);
		}));

		auto createList = [this, panel, showAll, callback](const std::string& searchName) {
			std::vector<std::pair<std::string, fs::path>> layouts;
			if (showAll) {
				layouts.emplace_back("", fs::path());
				for (const ContentPack& pack : engine->getContentPacks()) {
					auto files = getFiles(pack.folder / "layouts", false);
					for (const auto& file : files)
						layouts.emplace_back(pack.id, file);
				}
			}
			else {
				auto files = getFiles(currentPack.folder / "layouts", false);
				for (const auto& file : files)
					layouts.emplace_back(currentPack.id, file);
			}

			for (const auto& elem : layouts) {
				std::string actualName(elem.second.stem().string());
				if (actualName.find(searchName) == std::string::npos) continue;
				std::string fileName(elem.first + ':' + actualName);
				std::string displayName(actualName.empty() ? NOT_SET : showAll ? fileName : actualName);
				if (assets->getLayout(fileName) || elem.second.string().empty()) {
					auto button = std::make_shared<gui::Button>(util::str2wstr_utf8(displayName), glm::vec4(10.f), [this, elem, fileName, actualName, callback](gui::GUI*) {
						if (callback) callback(fileName);
						else createUILayoutEditor(elem.second, fileName, actualName, {});
					});
					button->setTextAlign(gui::Align::left);
					panel->add(removeList.emplace_back(button));
				}
			}
			setSelectable<gui::Button>(panel);
		};

		auto textBox = std::make_shared<gui::TextBox>(L"Search");
		textBox->setTextValidator([this, panel, createList, textBox](const std::wstring&) {
			clearRemoveList(panel);
			createList(util::wstr2str_utf8(textBox->getInput()));
			return true;
		});
		panel->add(textBox);

		createList("");

		return panel;
	}, column, posX);
}