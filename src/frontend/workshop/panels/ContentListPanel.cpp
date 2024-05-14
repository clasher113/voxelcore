#include "../WorkshopScreen.hpp"

#include "../../../assets/Assets.hpp"
#include "../../../content/Content.hpp"
#include "../../../graphics/core/Atlas.hpp"
#include "../../../items/ItemDef.hpp"
#include "../IncludeCommons.hpp"
#include "../WorkshopPreview.hpp"

void workshop::WorkShopScreen::createContentList(DefType type, unsigned int column, bool showAll,
	const std::function<void(const std::string&)>& callback, float posX)
{
	createPanel([this, type, showAll, callback]() {
		auto panel = std::make_shared<gui::Panel>(glm::vec2(200));
		panel->setScrollable(true);
		if (!showAll) {
			panel->add(std::make_shared<gui::Button>(L"Create " + util::str2wstr_utf8(getDefName(type)), glm::vec4(10.f), [this, type](gui::GUI*) {
				createDefActionPanel(DefAction::CREATE_NEW, type);
			}));
		}
		auto createList = [this, panel, type, showAll, callback](const std::string& searchName) {
			size_t size = (type == DefType::BLOCK ? indices->countBlockDefs() : indices->countItemDefs());
			std::vector<std::pair<std::string, std::string>> sorted;
			for (size_t i = 0; i < size; i++) {
				std::string fullName, actualName;
				if (type == DefType::ITEM) fullName = indices->getItemDef(static_cast<itemid_t>(i))->name;
				else if (type == DefType::BLOCK) fullName = indices->getBlockDef(static_cast<blockid_t>(i))->name;

				if (!showAll && fullName.substr(0, currentPack.id.length()) != currentPack.id) continue;
				actualName = fullName.substr(std::min(fullName.length(), currentPack.id.length() + 1));
				if (!searchName.empty() && (showAll ? fullName.find(searchName) : actualName.find(searchName)) == std::string::npos) continue;
				sorted.emplace_back(fullName, actualName);
			}
			std::sort(sorted.begin(), sorted.end(), [this, type, showAll](const decltype(sorted[0])& a, const decltype(sorted[0])& b) {
				if (type == DefType::ITEM) {
					auto hasFile = [this](const std::string& name) {
						return fs::is_regular_file(currentPack.folder / ContentPack::ITEMS_FOLDER / std::string(name + ".json"));
					};
					return hasFile(a.second) > hasFile(b.second) || (hasFile(a.second) == hasFile(b.second) &&
						(showAll ? a.first : a.second) < (showAll ? b.first : b.second));
				}
				if (showAll) return a.first < b.first;
				return a.second < b.second;
			});

			float width = panel->getSize().x;
			for (const auto& elem : sorted) {
				Atlas* contentAtlas = previewAtlas;
				std::string textureName("core:air");
				ItemDef& item = *content->findItem(elem.first);
				Block& block = *content->findBlock(elem.first);
				if (type == DefType::BLOCK) {
					textureName = elem.first;
				}
				else if (type == DefType::ITEM) {
					validateItem(assets, item);
					if (item.iconType == item_icon_type::block) {
						textureName = item.icon;
					}
					else if (item.iconType == item_icon_type::sprite) {
						contentAtlas = getAtlas(assets, item.icon); 
						textureName = getTexName(item.icon);
					}
				}
				auto button = std::make_shared<gui::IconButton>(glm::vec2(width, 32), showAll ? elem.first : elem.second,
					contentAtlas, textureName);

				if (type == DefType::BLOCK) {
					button->listenAction([this, elem, &block, callback](gui::GUI*) {
						if (callback) callback(elem.first);
						else {
							preview->setBlock(content->findBlock(elem.first));
							createBlockEditor(block);
						}
					});
				}
				else if (type == DefType::ITEM) {
					button->listenAction([this, elem, &item, callback](gui::GUI*) {
						if (callback) callback(item.name);
						else createItemEditor(item);
					});
				}
				panel->add(removeList.emplace_back(button));
			}
			setSelectable<gui::IconButton>(panel);
		};
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