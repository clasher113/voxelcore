#include "../WorkshopScreen.hpp"

#include "../../../items/ItemDef.hpp"
#include "../IncludeCommons.hpp"
#include "../WorkshopPreview.hpp"
#include "../WorkshopSerializer.hpp"

void workshop::WorkShopScreen::createBlockEditor(Block& block) {
	createPanel([this, &block]() {
		validateBlock(assets, block);

		std::string actualName(block.name.substr(currentPack.id.size() + 1));

		auto panel = std::make_shared<gui::Panel>(glm::vec2(200));

		panel->add(std::make_shared<gui::Label>(actualName));

		//createTextBox(panel, block.caption, L"Example Block");

		createFullCheckBox(panel, L"Light passing", block.lightPassing);
		createFullCheckBox(panel, L"Sky light passing", block.skyLightPassing);
		createFullCheckBox(panel, L"Obstacle", block.obstacle);
		createFullCheckBox(panel, L"Selectable", block.selectable);
		createFullCheckBox(panel, L"Replaceable", block.replaceable);
		createFullCheckBox(panel, L"Breakable", block.breakable);
		createFullCheckBox(panel, L"Grounded", block.grounded);
		createFullCheckBox(panel, L"Hidden", block.hidden);

		panel->add(std::make_shared<gui::Label>(L"Picking item"));
		auto pickingItemPanel = std::make_shared<gui::Panel>(glm::vec2(panel->getSize().x, 35.f));
		auto item = [this](std::string pickingItem) {
			return (content->findItem(pickingItem) == nullptr ? content->findItem("core:empty") : content->findItem(pickingItem));
		};
		auto atlas = [this, item, &block](std::string pickingItem) {
			ItemDef* i = item(pickingItem);
			if (i->iconType == item_icon_type::block) return previewAtlas;
			return getAtlas(assets, i->icon);
		};
		auto texName = [](ItemDef* item) {
			if (item->iconType == item_icon_type::none) return std::string("transparent");
			if (item->iconType == item_icon_type::block) return item->icon;
			return getTexName(item->icon);
		};
		auto rbutton = std::make_shared<gui::IconButton>(glm::vec2(panel->getSize().x, 35.f), texName(item(block.pickingItem)), atlas(block.pickingItem),
			texName(item(block.pickingItem)));
		rbutton->listenAction([this, rbutton, panel, texName, item, atlas, &block](gui::GUI*) {
			createContentList(DefType::ITEM, 5, true, [this, rbutton, texName, item, atlas, &block](std::string name) {
				block.pickingItem = name;
				rbutton->setIcon(atlas(block.pickingItem), texName(item(block.pickingItem)));
				rbutton->setText(item(block.pickingItem)->name);
				removePanels(5);
			}, panel->calcPos().x + panel->getSize().x);
		});
		panel->add(rbutton);

		auto texturePanel = std::make_shared<gui::Panel>(glm::vec2(panel->getSize().x, 35.f));
		texturePanel->setColor(glm::vec4(0.f));
		panel->add(texturePanel);

		const char* models[] = { "none", "block", "X", "aabb", "custom" };
		auto processModelChange = [this, texturePanel](Block& block) {
			clearRemoveList(texturePanel);
			if (block.model == BlockModel::custom) {
				createCustomModelEditor(block, 0, PrimitiveType::AABB);
			}
			else {
				createCustomModelEditor(block, 0, PrimitiveType::HITBOX);
				createTexturesPanel(texturePanel, 35.f, block.textureFaces, block.model);
			}
			texturePanel->cropToContent();
			preview->updateMesh();
		};

		auto button = std::make_shared<gui::Button>(L"Model: " + util::str2wstr_utf8(models[static_cast<size_t>(block.model)]), glm::vec4(10.f), gui::onaction());
		button->listenAction([&block, button, models, processModelChange](gui::GUI*) {
			size_t index = static_cast<size_t>(block.model) + 1;
			if (index >= std::size(models)) index = 0;
			button->setText(L"Model: " + util::str2wstr_utf8(models[index]));
			block.model = static_cast<BlockModel>(index);
			processModelChange(block);
		});
		panel->add(button);

		processModelChange(block);

		auto getRotationName = [](const Block& block) {
			if (!block.rotatable) return "none";
			return block.rotations.name.data();
		};

		button = std::make_shared<gui::Button>(L"Rotation: " + util::str2wstr_utf8(getRotationName(block)), glm::vec4(10.f), gui::onaction());
		button->listenAction([&block, getRotationName, button](gui::GUI*) {
			std::string rot = getRotationName(block);
			block.rotatable = true;
			if (rot == "none") {
				block.rotations = BlockRotProfile::PANE;
			}
			else if (rot == "pane") {
				block.rotations = BlockRotProfile::PIPE;
			}
			else if (rot == "pipe") {
				block.rotatable = false;
			}
			button->setText(L"Rotation: " + util::str2wstr_utf8(getRotationName(block)));
		});
		panel->add(button);

		panel->add(std::make_shared<gui::Label>("Draw group"));
		panel->add(createNumTextBox<ubyte>(block.drawGroup, L"0", 0, 255));
		createEmissionPanel(panel, block.emission);

		panel->add(std::make_shared<gui::Label>("Script file"));
		button = std::make_shared<gui::Button>(util::str2wstr_utf8(getScriptName(currentPack, block.scriptName)), glm::vec4(10.f), gui::onaction());
		button->listenAction([this, panel, button, actualName, &block](gui::GUI*) {
			createScriptList(5, panel->calcPos().x + panel->getSize().x, [this, button, actualName, &block](const std::string& string) {
				removePanels(5);
				std::string scriptName(getScriptName(currentPack, string));
				block.scriptName = (scriptName == NOT_SET ? (getScriptName(currentPack, actualName) == NOT_SET ? actualName : "") : scriptName);
				button->setText(util::str2wstr_utf8(scriptName));
			});
		});
		panel->add(button);

		panel->add(std::make_shared<gui::Label>("Block material"));
		button = std::make_shared<gui::Button>(util::str2wstr_utf8(block.material), glm::vec4(10.f), gui::onaction());
		button->listenAction([this, panel, button, &block](gui::GUI*) {
			createMaterialsList(true, 5, panel->calcPos().x + panel->getSize().x, [this, button, &block](const std::string& string) {
				removePanels(5);
				button->setText(util::str2wstr_utf8(block.material = string));
			});
		});
		panel->add(button);

		panel->add(std::make_shared<gui::Label>("UI layout"));
		button = std::make_shared<gui::Button>(util::str2wstr_utf8(getUILayoutName(assets, block.uiLayout)), glm::vec4(10.f), gui::onaction());
		button->listenAction([this, panel, button, &block](gui::GUI*) {
			createUILayoutList(true, 5, panel->calcPos().x + panel->getSize().x, [this, button, &block](const std::string& string) {
				removePanels(5);
				std::string layoutName(getUILayoutName(assets, string));
				block.uiLayout = (layoutName == NOT_SET ? (getUILayoutName(assets, block.name) == NOT_SET ? block.name : "") : layoutName);
				button->setText(util::str2wstr_utf8(layoutName));
			});
		});
		panel->add(button);

		panel->add(std::make_shared<gui::Label>("Inventory size"));
		panel->add(createNumTextBox<uint>(block.inventorySize, L"0 (no inventory)", 0, 64));

		panel->add(std::make_shared<gui::Button>(L"Save", glm::vec4(10.f), [this, &block, actualName](gui::GUI*) {
			saveBlock(block, currentPack.folder, actualName);
		}));
		panel->add(std::make_shared<gui::Button>(L"Rename", glm::vec4(10.f), [this, actualName](gui::GUI*) {
			createDefActionPanel(DefAction::RENAME, DefType::BLOCK, actualName);
		}));
		panel->add(std::make_shared<gui::Button>(L"Delete", glm::vec4(10.f), [this, actualName](gui::GUI*) {
			createDefActionPanel(DefAction::DELETE, DefType::BLOCK, actualName);
		}));

		return panel;
	}, 2);
}