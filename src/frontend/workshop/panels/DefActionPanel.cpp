#include "frontend/workshop/WorkshopScreen.hpp"

#include "coders/xml.hpp"
#include "items/ItemDef.hpp"
#include "graphics/ui/elements/Panel.hpp"
#include "graphics/ui/elements/Label.hpp"
#include "graphics/ui/elements/Textbox.hpp"
#include "graphics/ui/elements/Button.hpp"
#include "frontend/workshop/WorkshopSerializer.hpp"
#include "objects/EntityDef.hpp"
#include "objects/rigging.hpp"
#include "util/stringutil.hpp"
#include "voxels/Block.hpp"

#include <filesystem>

namespace fs = std::filesystem;

void workshop::WorkShopScreen::createDefActionPanel(ContentAction action, ContentType type, const std::string& name, bool reInitialize) {
	createPanel([this, action, type, name, reInitialize]() {
		gui::Panel& panel = *new gui::Panel(glm::vec2(200));

		const std::wstring buttonAct[] = { L"Create", L"Rename", L"Delete" };
		const std::wstring defName(util::str2wstr_utf8(getDefName(type)));

		panel << new gui::Label(buttonAct[static_cast<int>(action)] + L' ' + defName);
		if (type == ContentType::BLOCK && action == ContentAction::RENAME) {
			fs::path itemFile(currentPack.folder / getDefFolder(ContentType::ITEM) / (name + BLOCK_ITEM_SUFFIX + getDefFileFormat(ContentType::ITEM)));
			if (fs::is_regular_file(itemFile)) {
				panel << new gui::Label("This block has item file");
				panel << new gui::Label(itemFile.stem().string());
				panel << new gui::Label("Delete item file first");
				return std::ref(panel);
			};
		}
		gui::TextBox* nameInput = nullptr;
		gui::Button* uiRootButton = nullptr;
		if (action == ContentAction::CREATE_NEW && type == ContentType::UI_LAYOUT) {
			const std::wstring rootTypes[] = { L"panel", L"inventory", L"container" };
			panel << new gui::Label("Root type:");
			uiRootButton = new gui::Button(rootTypes[0], glm::vec4(10.f), gui::onaction());
			uiRootButton->listenAction([uiRootButton, rootTypes](gui::GUI*) {
				size_t index = 0;
				const std::wstring text(uiRootButton->getText());
				while (index < std::size(rootTypes)) {
					if (text == rootTypes[index++]) break;
				}
				if (index >= std::size(rootTypes)) index = 0;
				uiRootButton->setText(rootTypes[index]);
			});
			panel << uiRootButton;
		}
		if (action == ContentAction::DELETE) {
			panel << new gui::Label(name + "?");
		}
		else {
			nameInput = new gui::TextBox(L"example_" + defName);
			nameInput->setTooltipDelay(0.f);
			nameInput->setTextValidator([this, nameInput, type](const std::wstring& text) {
				std::wstring winput(nameInput->getInput());
				std::string input(util::wstr2str_utf8(winput));
				std::wstring tooltip;
				bool isOk = true;
				try {
					fs::exists(input);
				}
				catch (const std::exception&) {
					nameInput->setTooltip(L"Invalid file name");
					return false;
				}
				{
					std::wstring prefix(L"Id already in use: ");
					if (type == ContentType::BLOCK || type == ContentType::ITEM || type == ContentType::ENTITY) {
						auto blockIt = blocksList.find(input);
						auto itemIt = itemsList.find(input);
						auto entityIt = entityList.find(input);
						isOk = !(blockIt != blocksList.end() || itemIt != itemsList.end() || entityIt != entityList.end());
						if (blockIt != blocksList.end()) prefix.append(util::str2wstr_utf8("block." + blockIt->first));
						if (itemIt != itemsList.end()) prefix.append(util::str2wstr_utf8("item." + itemIt->first));
						if (entityIt != entityList.end()) prefix.append(util::str2wstr_utf8("entity." + entityIt->first));
					}
					else if (type == ContentType::UI_LAYOUT) {
						if (!(isOk = !fs::is_regular_file(currentPack.folder / getDefFolder(type) / (input + ".xml"))))
							prefix.append(winput);
					}
					else if (type == ContentType::SKELETON) {
						if (!(isOk = skeletons.find(input) == skeletons.end()))
							prefix.append(winput);
					}
					if (!isOk) tooltip = prefix;
				}
				if (isOk) {
					if (!(isOk = !winput.empty()))
						tooltip = L"Input must not be empty";
					else if (!(isOk = std::all_of(winput.begin(), winput.end(),
						[](const wchar_t c) { return c < 255 && (iswalnum(c) || c == '_'); }))) {
						tooltip = L"Id has illegal character";
					}
				}
				nameInput->setTooltip(tooltip);
				return isOk;
			});
			panel << nameInput;
		}
		panel << new gui::Button(buttonAct[static_cast<int>(action)], glm::vec4(10.f), [this, nameInput, uiRootButton, action, name, type, reInitialize](gui::GUI*) {
			std::string input;
			if (nameInput) {
				if (!nameInput->validate()) return;
				input = util::wstr2str_utf8(nameInput->getInput());
			}
			if (reInitialize && showUnsaved()) return;
			const fs::path path(currentPack.folder / getDefFolder(type));
			const std::string fileFormat(getDefFileFormat(type));
			if (!fs::is_directory(path)) fs::create_directory(path);
			if (action == ContentAction::CREATE_NEW) {
				if (type == ContentType::UI_LAYOUT) {
					auto doc = std::make_shared<xml::Document>("1.0", "UTF-8");
					auto root = std::make_unique<xml::Node>(util::wstr2str_utf8(uiRootButton->getText()));
					for (const auto& elem : uiElementsArgs.at(util::wstr2str_utf8(uiRootButton->getText())).attrTemplate) {
						root->set(elem.first, elem.second);
					}
					doc->setRoot(std::move(root));
					saveDocument(*doc, currentPack.folder, input);
				}
				else if (type == ContentType::BLOCK) saveBlock(Block(':' + input), currentPack.folder, input, nullptr, "");
				else if (type == ContentType::ITEM) saveItem(ItemDef(':' + input), currentPack.folder, input, nullptr, "");
				else if (type == ContentType::ENTITY) saveEntity(EntityDef(':' + input), currentPack.folder, input, nullptr, "");
				else if (type == ContentType::SKELETON) saveSkeleton(rigging::Bone(0, "", "", {}, glm::vec3(0.f)), currentPack.folder, input);
			}
			else if (action == ContentAction::RENAME) {
				fs::rename(path / (name + fileFormat), path / (input + fileFormat));
			}
			else if (action == ContentAction::DELETE) {
				if (type == ContentType::BLOCK) {
					fs::path blockItemFilePath(currentPack.folder / getDefFolder(ContentType::ITEM) / (name + BLOCK_ITEM_SUFFIX + fileFormat));
					if (fs::is_regular_file(blockItemFilePath)) fs::remove(blockItemFilePath);
				}
				fs::remove(path / (name + fileFormat));
			}
			if (reInitialize) initialize();
			if (type == ContentType::ITEM || type == ContentType::BLOCK) createContentList(type);
			else if (type == ContentType::UI_LAYOUT) createUILayoutList();
			else if (type == ContentType::ENTITY) createEntitiesList();
			else if (type == ContentType::SKELETON) createSkeletonList();
		});

		return std::ref(panel);
	}, 2);
}