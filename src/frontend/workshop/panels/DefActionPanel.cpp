#include "../WorkshopScreen.hpp"

#include "../../../coders/xml.hpp"
#include "../../../items/ItemDef.hpp"
#include "../IncludeCommons.hpp"
#include "../WorkshopSerializer.hpp"

void workshop::WorkShopScreen::createDefActionPanel(DefAction action, DefType type, const std::string& name, bool reInitialize) {
	createPanel([this, action, type, name, reInitialize]() {
		auto panel = std::make_shared<gui::Panel>(glm::vec2(200));

		const std::wstring buttonAct[] = { L"Create", L"Rename", L"Delete" };
		const std::wstring defName(util::str2wstr_utf8(getDefName(type)));

		panel->add(std::make_shared<gui::Label>(buttonAct[static_cast<int>(action)] + L' ' + defName));
		std::shared_ptr<gui::TextBox> nameInput;
		std::shared_ptr<gui::Button> uiRootButton;
		if (action == DefAction::CREATE_NEW && type == DefType::UI_LAYOUT) {
			const std::wstring rootTypes[] = { L"panel", L"inventory", L"container" };
			panel->add(std::make_shared<gui::Label>("Root type:"));
			uiRootButton = std::make_shared<gui::Button>(rootTypes[0], glm::vec4(10.f), gui::onaction());
			uiRootButton->listenAction([uiRootButton, rootTypes](gui::GUI*) {
				size_t index = 0;
				const std::wstring text(uiRootButton->getText());
				while (index < std::size(rootTypes)) {
					if (text == rootTypes[index++]) break;
				}
				if (index >= std::size(rootTypes)) index = 0;
				uiRootButton->setText(rootTypes[index]);
			});
			panel->add(uiRootButton);
		}
		if (action == DefAction::DELETE) {
			panel->add(std::make_shared<gui::Label>(name + "?"));
		}
		else {
			nameInput = std::make_shared<gui::TextBox>(L"example_" + defName);
			nameInput->setTextValidator([this, nameInput, type](const std::wstring& text) {
				std::string input(util::wstr2str_utf8(nameInput->getInput()));
				bool found = (type == DefType::BLOCK || type == DefType::ITEM ?
					(blocksList.find(input) == blocksList.end() && itemsList.find(input) == itemsList.end()) :
					!fs::is_regular_file(currentPack.folder / getDefFolder(type) / (input + ".xml")));
				return found && util::is_valid_filename(nameInput->getInput()) && !input.empty();
			});
			panel->add(nameInput);
		}
		panel->add(std::make_shared<gui::Button>(buttonAct[static_cast<int>(action)], glm::vec4(10.f), [this, nameInput, uiRootButton, action, name, type, reInitialize](gui::GUI*) {
			std::string input;
			if (nameInput) {
				if (!nameInput->validate()) return;
				input = util::wstr2str_utf8(nameInput->getInput());
			}
			fs::path path(currentPack.folder / getDefFolder(type));
			const std::string fileFormat(getDefFileFormat(type));
			if (!fs::is_directory(path)) fs::create_directory(path);
			if (action == DefAction::CREATE_NEW) {
				if (type == DefType::UI_LAYOUT) {
					xml::xmldocument doc = std::make_shared<xml::Document>("1.0", "UTF-8");
					auto root = std::make_shared<xml::Node>(util::wstr2str_utf8(uiRootButton->getText()));
					for (const auto& elem : uiElementsArgs.at(util::wstr2str_utf8(uiRootButton->getText())).attrTemplate) {
						root->set(elem.first, elem.second);
					}
					doc->setRoot(root);
					saveDocument(doc, currentPack.folder, input);
				}
				else if (type == DefType::BLOCK) saveBlock(Block(""), currentPack.folder, input);
				else if (type == DefType::ITEM) saveItem(ItemDef(""), currentPack.folder, input);
			}
			else if (action == DefAction::RENAME) {
				fs::rename(path / (name + fileFormat), path / (input + fileFormat));
			}
			else if (action == DefAction::DELETE) {
				if (type == DefType::BLOCK) {
					fs::path blockItemFilePath(currentPack.folder / getDefFolder(DefType::ITEM) / fs::path(name + BLOCK_ITEM_SUFFIX + fileFormat));
					if (fs::is_regular_file(blockItemFilePath)) fs::remove(blockItemFilePath);
				}
				fs::remove(path / (name + fileFormat));
			}
			if (reInitialize) initialize();
			if (type == DefType::ITEM || type == DefType::BLOCK) createContentList(type, 1);
			else createUILayoutList();
		}));

		return panel;
	}, 2);
}