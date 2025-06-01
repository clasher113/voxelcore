#include "frontend/workshop/WorkshopScreen.hpp"

#include "content/Content.hpp"
#include "items/ItemDef.hpp"
#include "graphics/ui/elements/Panel.hpp"
#include "graphics/ui/elements/Button.hpp"
#include "graphics/ui/elements/Label.hpp"
#include "graphics/ui/elements/TextBox.hpp"
#include "frontend/workshop/gui_elements/BasicElements.hpp"
#include "frontend/workshop/gui_elements/IconButton.hpp"
#include "frontend/workshop/BlockModelConverter.hpp"
#include "frontend/workshop/WorkshopPreview.hpp"
#include "frontend/workshop/WorkshopSerializer.hpp"
#include "util/stringutil.hpp"
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
		createFullCheckBox(panel, L"Replaceable", block.replaceable, L"Can the block be replaced with other. Examples of replaceable blocks: air, flower, water");
		createFullCheckBox(panel, L"Shadeless", block.shadeless, L"Does block model have shading");
		createFullCheckBox(panel, L"Ambient Occlusion", block.ambientOcclusion, L"Does block model have vertex-based AO effect");
		createFullCheckBox(panel, L"Breakable", block.breakable, L"Can player destroy the block");
		createFullCheckBox(panel, L"Grounded", block.grounded, L"Can the block exist without physical support be a solid block below");
		createFullCheckBox(panel, L"Hidden", block.hidden, L"Turns off block item generation");

		panel << new gui::Label(L"Parent block");
		auto parentIcoName = [](const Block* const parent) {
			return parent ? parent->name : NOT_SET;
		};
		auto parentIcoTexName = [](const Block* const parent) {
			return BLOCKS_PREVIEW_ATLAS + ':' + (parent ? parent->name : CORE_AIR);
		};
		BackupData& backupData = blocksList[actualName];
		const Block* currentParent = content->blocks.find(backupData.parent);

		gui::IconButton& parentBlock = *new gui::IconButton(assets, 35.f, parentIcoName(currentParent), parentIcoTexName(currentParent));
		parentBlock.listenAction([=, &panel, &block](gui::GUI*) {
			if (showUnsaved()) return;
			createContentList(ContentType::BLOCK, true, 5, panel.calcPos().x + panel.getSize().x, [=, &block](std::string string) {
				const std::string name = block.name;
				if (string == CORE_AIR || string == block.name) string.clear();
				saveBlock(block, currentPack.folder, currentParent, string);
				removePanels(1);
				initialize();
				createContentList(ContentType::BLOCK);
				createBlockEditor(*content->blocks.getDefs().at(name));
			});
		});
		panel << parentBlock;

		panel << new gui::Label("Picking item");

		auto itemTexName = [this](const std::string& itemName) -> std::string {
			const ItemDef* const item = content->items.find(itemName);
			if (item == nullptr || item->iconType == ItemIconType::NONE) return "blocks:transparent";
			if (item->iconType == ItemIconType::BLOCK) return BLOCKS_PREVIEW_ATLAS + ':' + item->icon;
			return item->icon;
		};

		gui::IconButton* iconButton = new gui::IconButton(assets, 35.f, block.pickingItem, itemTexName(block.pickingItem));
		iconButton->listenAction([=, &panel, &block](gui::GUI*) {
			createContentList(ContentType::ITEM, true, 5, panel.calcPos().x + panel.getSize().x, [=, &block](const std::string& name) {
				block.pickingItem = name;
				iconButton->setIcon(assets, itemTexName(block.pickingItem));
				iconButton->setText(name);
				removePanels(5);
			});
		});
		panel << iconButton;

		panel << new gui::Label("Surface replacement");
		panel << (iconButton = new gui::IconButton(assets, 35.f, block.surfaceReplacement,
			BLOCKS_PREVIEW_ATLAS + ':' + block.surfaceReplacement));
		iconButton->setTooltip(L"Block will be used instead of this if generated on surface");
		iconButton->listenAction([=, &panel, &block](gui::GUI*) {
			createContentList(ContentType::BLOCK, true, 5, panel.calcPos().x + panel.getSize().x, [=, &block](const std::string& name) {
				block.surfaceReplacement = name;
				iconButton->setIcon(assets, BLOCKS_PREVIEW_ATLAS + ':' + block.surfaceReplacement);
				iconButton->setText(block.surfaceReplacement);
				removePanels(5);
			});
		});

		gui::Panel& texturePanel = *new gui::Panel(glm::vec2(panel.getSize().x, 35.f), glm::vec4(0.f));
		texturePanel.setColor(glm::vec4(0.f));
		panel << texturePanel;

		auto openAdditionalPanel = [=, &texturePanel, &panel](Block& block) {
			removeRemovable(texturePanel);
			if (block.model == BlockModel::custom) {
				createAdditionalBlockEditorPanel(block, 0, PrimitiveType::AABB);
			}
			else {
				createAdditionalBlockEditorPanel(block, 0, PrimitiveType::HITBOX);
				createTexturesPanel(texturePanel, 35.f, block.textureFaces.data(), block.model);
			}
			texturePanel.cropToContent();
			panel.refresh();
			preview->updateMesh();
		};

		gui::Button* button = new gui::Button(L"Model: " + util::str2wstr_utf8(to_string(block.model)), glm::vec4(10.f), gui::onaction());
		button->listenAction([&block, button, openAdditionalPanel](gui::GUI*) {
			incrementEnumClass(block.model, 1);
			if (block.model > BlockModel::custom) block.model = BlockModel::none;
			button->setText(L"Model: " + util::str2wstr_utf8(to_string(block.model)));
			openAdditionalPanel(block);
		});
		panel << button;

		openAdditionalPanel(block);

		auto getRotationName = [](const Block& block) {
			if (!block.rotatable) return "none";
			return block.rotations.name.data();
		};

		panel << (button = new gui::Button(L"Rotation: " + util::str2wstr_utf8(getRotationName(block)), glm::vec4(10.f), gui::onaction()));
		button->listenAction([&block, getRotationName, button](gui::GUI*) {
			std::string rot = getRotationName(block);
			block.rotatable = true;
			if (rot == "none") {
				block.rotations = BlockRotProfile::PANE;
			}
			else if (rot == BlockRotProfile::PANE_NAME) {
				block.rotations = BlockRotProfile::PIPE;
			}
			else if (rot == BlockRotProfile::PIPE_NAME) {
				block.rotatable = false;
			}
			button->setText(L"Rotation: " + util::str2wstr_utf8(getRotationName(block)));
		});

		panel << (button = new gui::Button(L"Culling mode: " + util::str2wstr_utf8(to_string(block.culling)), glm::vec4(10.f), gui::onaction()));
		button->listenAction([&block, button](gui::GUI*) {
			incrementEnumClass(block.culling, 1);
			if (block.culling > CullingMode::DISABLED) block.culling = CullingMode::DEFAULT;
			button->setText(L"Culling mode: " + util::str2wstr_utf8(to_string(block.culling)));
		});

		auto overlayTextureName = [](const std::string& overlayTexture) {
			if (overlayTexture.empty()) return NOT_SET;
			return overlayTexture;
		};
		auto overlayTexture = [](const std::string& overlayTexture) -> std::string {
			if (overlayTexture.empty()) return "blocks:transparent";
			return overlayTexture;
		};
		
		panel << new gui::Label("Overlay texture");
		panel << (iconButton = new gui::IconButton(assets, 35.f, overlayTextureName(block.overlayTexture), overlayTexture(block.overlayTexture)));
		iconButton->listenAction([=, &block](gui::GUI*) {
			createTextureList(35.f, 5, { ContentType::BLOCK, ContentType::ITEM }, iconButton->calcPos().x + iconButton->getSize().x, true,
				[=, &block](std::string texName) {
				if (texName == "blocks:transparent") texName = NOT_SET;
				block.overlayTexture = texName == NOT_SET ? "" : texName;
				removePanel(5);
				deselect(*button);
				iconButton->setIcon(assets, overlayTexture(block.overlayTexture));
				iconButton->setText(overlayTextureName(block.overlayTexture));
			});
		});

		panel << new gui::Label("Draw group (0 - 255)");
		panel << createNumTextBox<ubyte>(block.drawGroup, L"0", 0, 0, 255);
		createEmissionPanel(panel, block.emission);

		panel << new gui::Label("Block size (1 - 5)");
		panel << createVectorPanel(block.size, glm::i8vec3(1), glm::i8vec3(5), panel.getSize().x, InputMode::TEXTBOX, [=, &block]() {
			preview->setBlockSize(block.size);
		});

		panel << new gui::Label("Script file");
		panel << (button = new gui::Button(util::str2wstr_utf8(getScriptName(currentPack, block.scriptName)), glm::vec4(10.f), gui::onaction()));
		button->listenAction([this, &panel, button, actualName, &block](gui::GUI*) {
			createScriptList(5, panel.calcPos().x + panel.getSize().x, [this, button, actualName, &block](const std::string& string) {
				removePanels(5);
				std::string scriptName(getScriptName(currentPack, string));
				block.scriptName = (scriptName == NOT_SET ? (getScriptName(currentPack, actualName) == NOT_SET ? actualName : "") : scriptName);
				button->setText(util::str2wstr_utf8(scriptName));
			});
		});

		panel << new gui::Label("Block material");
		panel << (button = new gui::Button(util::str2wstr_utf8(block.material), glm::vec4(10.f), gui::onaction()));
		button->listenAction([this, &panel, button, &block](gui::GUI*) {
			createMaterialsList(true, 5, panel.calcPos().x + panel.getSize().x, [this, button, &block](const std::string& string) {
				removePanels(5);
				button->setText(util::str2wstr_utf8(block.material = string));
			});
		});

		panel << new gui::Label("UI layout");
		panel << (button = new gui::Button(util::str2wstr_utf8(getUILayoutName(assets, block.uiLayout)), glm::vec4(10.f), gui::onaction()));
		button->listenAction([this, &panel, button, &block](gui::GUI*) {
			createUILayoutList(true, 5, panel.calcPos().x + panel.getSize().x, [this, button, &block](const std::string& string) {
				removePanels(5);
				std::string layoutName(getUILayoutName(assets, string));
				block.uiLayout = (layoutName == NOT_SET ? (getUILayoutName(assets, block.name) == NOT_SET ? block.name : "") : layoutName);
				button->setText(util::str2wstr_utf8(layoutName));
			});
		});

		panel << new gui::Label("Inventory size");
		panel << createNumTextBox<uint>(block.inventorySize, L"0 - no inventory", 0, 0, std::numeric_limits<decltype(block.inventorySize)>::max());

		panel << new gui::Label("Tick interval (1 - 20)");
		panel << createNumTextBox<uint>(block.tickInterval, L"1 - 20tps, 2 - 10tps", 0, 1, 20);
		
		panel << new gui::Button(L"Save", glm::vec4(10.f), [=, &block, &backupData](gui::GUI*) {
			const std::string parentName = currentParent ? currentParent->name : "";
			backupData.serialized = stringify(block, currentParent, parentName);
			saveBlock(block, currentPack.folder, currentParent, parentName);
			updateIcons();
		});

		bool isParent = false;
		std::wstring parentOf = L"Action impossible: Parent of ";
		for (const auto& [name, data] : blocksList) {
			if (data.parent == block.name) {
				parentOf.append(util::str2wstr_utf8(name) + L'\n');
				isParent = true;
			}
		}

		panel << (button = new gui::Button(L"Rename", glm::vec4(10.f), gui::onaction()));
		if (isParent) button->setTooltip(parentOf);
		else button->listenAction([this, actualName](gui::GUI*) {
			createDefActionPanel(ContentAction::RENAME, ContentType::BLOCK, actualName);
		});

		panel << (button = new gui::Button(L"Delete", glm::vec4(10.f), gui::onaction()));
		if (isParent) button->setTooltip(parentOf);
		else button->listenAction([this, actualName](gui::GUI*) {
			createDefActionPanel(ContentAction::DELETE, ContentType::BLOCK, actualName);
		});

		return std::ref(panel);
	}, 2);
}