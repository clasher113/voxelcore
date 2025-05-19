#include "frontend/workshop/WorkshopScreen.hpp"

#include "voxels/Block.hpp"
#include "graphics/ui/elements/Panel.hpp"
#include "graphics/ui/elements/Button.hpp"
#include "window/Window.hpp"
#include "frontend/workshop/WorkshopUtils.hpp"
#include "frontend/workshop/WorkshopPreview.hpp"
#include "frontend/workshop/gui_elements/TabsContainer.hpp"

using namespace workshop;

void WorkShopScreen::createAdditionalBlockEditorPanel(Block& block, size_t index, PrimitiveType type) {

	preview->setBlockSize(block.size);

	createPanel([this, &block, index, type]() {
		gui::Panel& mainPanel = *new gui::Panel(glm::vec2());
		mainPanel.setScrollable(false);
		mainPanel.listenInterval(0.01f, [&mainPanel]() {
			int newWidth = Window::width - mainPanel.calcPos().x - 2.f;
			if (mainPanel.getSize().x != newWidth)
				mainPanel.setSize(glm::vec2(newWidth, mainPanel.getSize().y));
		});
		gui::TabsContainer& tabsContainer = *new gui::TabsContainer(glm::vec2());
		mainPanel << tabsContainer;

		gui::Panel* panel = new gui::Panel(glm::vec2());
		panel->setColor(glm::vec4(0.f));
		createPrimitiveEditor(*panel, block, index, PrimitiveType::HITBOX);
		gui::Button* hitboxButton = tabsContainer.addTab("hitbox", L"Hitbox", panel);
		hitboxButton->listenAction([=](gui::GUI*) {
			preview->setCurrentPrimitive(PrimitiveType::HITBOX);
		});

		if (block.model == BlockModel::custom) {
			panel = new gui::Panel(glm::vec2());
			panel->setColor(glm::vec4(0.f));
			createPrimitiveEditor(*panel, block, index, PrimitiveType::AABB);
			gui::Button* modelButton = tabsContainer.addTab("model", L"Model", panel);
			modelButton->listenAction([this](gui::GUI*) {
				preview->setCurrentPrimitive(modelPrimitive);
			});
			tabsContainer.switchTo("model");
		}

		panel = new gui::Panel(glm::vec2());
		panel->setColor(glm::vec4(0.f));
		createPropertyEditor(*panel, block.properties, definedProps);
		tabsContainer.addTab("props", L"Custom Properties", panel);

		panel = new gui::Panel(glm::vec2());
		panel->setColor(glm::vec4(0.f));
		createBlockFieldsEditor(*panel, block.dataStruct);
		tabsContainer.addTab("fields", L"Fields", panel);

		panel = new gui::Panel(glm::vec2());
		panel->setColor(glm::vec4(0.f));
		createBlockParticlesEditor(*panel, block.particles);
		tabsContainer.addTab("particles", L"Particles", panel);

		return std::ref(mainPanel);
	}, 3);
}