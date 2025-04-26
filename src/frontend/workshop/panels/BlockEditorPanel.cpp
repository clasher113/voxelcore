#include "../WorkshopScreen.hpp"

#include "content/Content.hpp"
#include "items/ItemDef.hpp"
#include "../BlockModelConverter.hpp"
#include "../IncludeCommons.hpp"
#include "../WorkshopPreview.hpp"
#include "../WorkshopSerializer.hpp"
#include "core_defs.hpp"

void workshop::WorkShopScreen::createBlockEditor(Block& block) {
	createPanel([this, &block]() {
		validateBlock(assets, block);
		preview->setBlock(&block);
		preview->updateCache();

		const std::string actualName(getDefName(block.name));

		gui::Panel& panel = *new gui::Panel(glm::vec2(settings.blockEditorWidth));

		panel << new gui::Label(actualName);

		panel << new gui::Label("Caption");
		createTextBox(panel, block.caption, L"Example block");

		createFullCheckBox(panel, L"Light passing", block.lightPassing, L"Does the block passing lights into itself");
		createFullCheckBox(panel, L"Sky light passing", block.skyLightPassing, L"Does the block passing top-down sky lights into itself");
		createFullCheckBox(panel, L"Translucent", block.translucent, L"Block has semi-transparent texture");
		createFullCheckBox(panel, L"Obstacle", block.obstacle, L"Is the block a physical obstacle");
		createFullCheckBox(panel, L"Selectable", block.selectable, L"Can the block be selected");
		createFullCheckBox(panel, L"Replaceable", block.replaceable, L"Can the block be replaced with other. \n Examples of replaceable blocks: air, flower, water");
		createFullCheckBox(panel, L"Shadeless", block.shadeless, L"Does block model have shading");
		createFullCheckBox(panel, L"Ambient Occlusion", block.ambientOcclusion, L"Does block model have vertex-based AO effect");
		createFullCheckBox(panel, L"Breakable", block.breakable, L"Can player destroy the block");
		createFullCheckBox(panel, L"Grounded", block.grounded, L"Can the block exist without physical support be a solid block below");
		createFullCheckBox(panel, L"Hidden", block.hidden, L"Turns off block item generation");

		panel << new gui::Label(L"Parent block");
		auto parentIcoName = [](const Block* const parent) {
			return parent ? parent->name : NOT_SET;
		};
		auto parentIcoTexName = [parentIcoName](const Block* const parent) {
			return parentIcoName(parent) == NOT_SET ? CORE_AIR : parent->name;
		};
		BackupData& backupData = blocksList[actualName];
		const Block* currentParent = content->blocks.find(backupData.currentParent);
		const Block* newParent = content->blocks.find(backupData.newParent);

		gui::IconButton& parentBlock = *new gui::IconButton(glm::vec2(panel.getSize().x, 35.f), parentIcoName(newParent), previewAtlas, parentIcoTexName(newParent));
		parentBlock.listenAction([=, &panel, &backupData, &parentBlock, &block](gui::GUI*) {
			createContentList(ContentType::BLOCK, true, 5, panel.calcPos().x + panel.getSize().x, [=, &backupData, &parentBlock, &block](const std::string& string) {
				const Block* parent = content->blocks.find(string);
				if (parent->name == CORE_AIR || parent->name == block.name) parent = nullptr;
				backupData.newParent = parent ? string : "";
				parentBlock.setIcon(previewAtlas, parentIcoTexName(parent));
				parentBlock.setText(parentIcoName(parent));
				removePanels(5);
			});
		});
		panel << parentBlock;

		panel << new gui::Label(L"Picking item");
		auto item = [this](const std::string& pickingItem) {
			return (content->items.find(pickingItem) == nullptr ? content->items.find("core:empty") : content->items.find(pickingItem));
		};
		auto atlas = [this, item, &block](const std::string& pickingItem) {
			const ItemDef* const i = item(pickingItem);
			if (i->iconType == ItemIconType::BLOCK) return previewAtlas;
			return getAtlas(assets, i->icon);
		};
		auto texName = [](const ItemDef* const item) {
			if (item->iconType == ItemIconType::NONE) return std::string("transparent");
			if (item->iconType == ItemIconType::BLOCK) return item->icon;
			return getTexName(item->icon);
		};
		gui::IconButton& pickingItem = *new gui::IconButton(glm::vec2(panel.getSize().x, 35.f), block.pickingItem, atlas(block.pickingItem),
			texName(item(block.pickingItem)));
		pickingItem.listenAction([this, &pickingItem, &panel, texName, item, atlas, &block](gui::GUI*) {
			createContentList(ContentType::ITEM, true, 5, panel.calcPos().x + panel.getSize().x, [this, &pickingItem, texName, item, atlas, &block](std::string name) {
				block.pickingItem = name;
				pickingItem.setIcon(atlas(block.pickingItem), texName(item(block.pickingItem)));
				pickingItem.setText(name);
				removePanels(5);
			});
		});
		panel << pickingItem;

		gui::Panel& texturePanel = *new gui::Panel(glm::vec2(panel.getSize().x, 35.f), glm::vec4(0.f));
		texturePanel.setColor(glm::vec4(0.f));
		panel << texturePanel;

		auto processModelChange = [this, &texturePanel, &panel](Block& block) {
			removeRemovable(texturePanel);
			if (block.model == BlockModel::custom) {
				createCustomModelEditor(block, 0, PrimitiveType::AABB);
				texturePanel << markRemovable(new gui::Button(L"Import model", glm::vec4(10), [this, &panel, &block](gui::GUI*) {
					createBlockConverterPanel(block, panel.getPos().x + panel.getSize().x);
				}));
			}
			else {
				createCustomModelEditor(block, 0, PrimitiveType::HITBOX);
				createTexturesPanel(texturePanel, 35.f, block.textureFaces.data(), block.model);
			}
			texturePanel.cropToContent();
			panel.refresh();
			preview->updateMesh();
		};

		gui::Button* button = new gui::Button(L"Model: " + util::str2wstr_utf8(to_string(block.model)), glm::vec4(10.f), gui::onaction());
		button->listenAction([&block, button, processModelChange](gui::GUI*) {
			incrementEnumClass(block.model, 1);
			if (block.model > BlockModel::custom) block.model = BlockModel::none;
			button->setText(L"Model: " + util::str2wstr_utf8(to_string(block.model)));
			processModelChange(block);
		});
		panel << button;

		processModelChange(block);

		auto getRotationName = [](const Block& block) {
			if (!block.rotatable) return "none";
			return block.rotations.name.data();
		};

		button = new gui::Button(L"Rotation: " + util::str2wstr_utf8(getRotationName(block)), glm::vec4(10.f), gui::onaction());
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
		panel << button;

		panel << new gui::Label("Draw group (0 - 255)");
		panel << createNumTextBox<ubyte>(block.drawGroup, L"0", 0, 255);
		createEmissionPanel(panel, block.emission);

		panel << new gui::Label("Block size (0 - 5)");
		panel << createVectorPanel(block.size, glm::i8vec3(0), glm::i8vec3(5), panel.getSize().x, InputMode::TEXTBOX, [processModelChange, &block]() {
			processModelChange(block);
		});

		panel << new gui::Label("Script file");
		button = new gui::Button(util::str2wstr_utf8(getScriptName(currentPack, block.scriptName)), glm::vec4(10.f), gui::onaction());
		button->listenAction([this, &panel, button, actualName, &block](gui::GUI*) {
			createScriptList(5, panel.calcPos().x + panel.getSize().x, [this, button, actualName, &block](const std::string& string) {
				removePanels(5);
				std::string scriptName(getScriptName(currentPack, string));
				block.scriptName = (scriptName == NOT_SET ? (getScriptName(currentPack, actualName) == NOT_SET ? actualName : "") : scriptName);
				button->setText(util::str2wstr_utf8(scriptName));
			});
		});
		panel << button;

		panel << new gui::Label("Block material");
		button = new gui::Button(util::str2wstr_utf8(block.material), glm::vec4(10.f), gui::onaction());
		button->listenAction([this, &panel, button, &block](gui::GUI*) {
			createMaterialsList(true, 5, panel.calcPos().x + panel.getSize().x, [this, button, &block](const std::string& string) {
				removePanels(5);
				button->setText(util::str2wstr_utf8(block.material = string));
			});
		});
		panel << button;

		panel << new gui::Label("UI layout");
		button = new gui::Button(util::str2wstr_utf8(getUILayoutName(assets, block.uiLayout)), glm::vec4(10.f), gui::onaction());
		button->listenAction([this, &panel, button, &block](gui::GUI*) {
			createUILayoutList(true, 5, panel.calcPos().x + panel.getSize().x, [this, button, &block](const std::string& string) {
				removePanels(5);
				std::string layoutName(getUILayoutName(assets, string));
				block.uiLayout = (layoutName == NOT_SET ? (getUILayoutName(assets, block.name) == NOT_SET ? block.name : "") : layoutName);
				button->setText(util::str2wstr_utf8(layoutName));
			});
		});
		panel << button;

		panel << new gui::Label("Inventory size");
		panel << createNumTextBox<uint>(block.inventorySize, L"0 - no inventory", 0, std::numeric_limits<decltype(block.inventorySize)>::max());

		panel << new gui::Label("Tick interval (1 - 20)");
		panel << createNumTextBox<uint>(block.tickInterval, L"1 - 20tps, 2 - 10tps", 1, 20);

		panel << new gui::Button(L"Save", glm::vec4(10.f), [this, &block, actualName, currentParent, &backupData](gui::GUI*) {
			backupData.string = stringify(toJson(block, actualName, currentParent, backupData.newParent), false);
			saveBlock(block, currentPack.folder, actualName, currentParent, backupData.newParent);
		});
		panel << new gui::Button(L"Rename", glm::vec4(10.f), [this, actualName](gui::GUI*) {
			createDefActionPanel(ContentAction::RENAME, ContentType::BLOCK, actualName);
		});
		panel << new gui::Button(L"Delete", glm::vec4(10.f), [this, actualName](gui::GUI*) {
			createDefActionPanel(ContentAction::DELETE, ContentType::BLOCK, actualName);
		});

		return std::ref(panel);
	}, 2);
}