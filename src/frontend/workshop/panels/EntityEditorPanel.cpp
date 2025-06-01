#include "frontend/workshop/WorkshopScreen.hpp"

#include "assets/Assets.hpp"
#include "content/Content.hpp"
#include "objects/EntityDef.hpp"
#include "graphics/ui/elements/Panel.hpp"
#include "graphics/ui/elements/Label.hpp"
#include "graphics/ui/elements/Button.hpp"
#include "graphics/ui/elements/CheckBox.hpp"
#include "graphics/ui/elements/Image.hpp"
#include "graphics/ui/elements/TextBox.hpp"
#include "frontend/workshop/gui_elements/BasicElements.hpp"
#include "frontend/workshop/WorkshopSerializer.hpp"
#include "frontend/workshop/WorkshopPreview.hpp"
#include "frontend/workshop/gui_elements/TabsContainer.hpp"
#include "graphics/core/Texture.hpp"
#include "util/stringutil.hpp"
#include "window/Window.hpp"
#include "constants.hpp"

using namespace workshop;

void WorkShopScreen::createEntityEditorPanel(EntityDef& entity) {
	createPanel([this, &entity]() {
		const std::string actualName(getDefName(entity.name));

		gui::Panel& panel = *new gui::Panel(glm::vec2(settings.itemEditorWidth));

		panel << new gui::Label(actualName);

		createFullCheckBox(panel, L"Blocking", entity.blocking, L"Does entity prevent blocks setup");
		gui::Panel& savingPanel = *new gui::Panel(panel.getSize(), glm::vec4(0.f));
		savingPanel.setColor(glm::vec4(0.5f, 0.5f, 0.5f, 0.25f));
		auto processSavingChange = [&entity, &savingPanel, &panel]() {
			removeRemovable(savingPanel);
			if (entity.save.enabled) {
				savingPanel << markRemovable(new gui::Label("Saving:"));
				markRemovable(createFullCheckBox(savingPanel, L"Skeleton textures", entity.save.skeleton.textures));
				markRemovable(createFullCheckBox(savingPanel, L"Skeleton pose", entity.save.skeleton.pose));
				markRemovable(createFullCheckBox(savingPanel, L"Body settings", entity.save.body.settings));
				markRemovable(createFullCheckBox(savingPanel, L"Body velocity", entity.save.body.velocity));
			}
			savingPanel.cropToContent();
			panel.refresh();
		};
		createFullCheckBox(panel, L"Saving entity", entity.save.enabled).setConsumer([&check = entity.save.enabled, processSavingChange](bool checked) {
			check = checked;
			processSavingChange();
		});
		panel << savingPanel;
		processSavingChange();

		auto parentName = [](const EntityDef* const parent) {
			return util::str2wstr_utf8(parent ? parent->name : NOT_SET);
		};

		BackupData& backupData = entityList[actualName];
		const EntityDef* currentParent = content->entities.find(backupData.parent);

		panel << new gui::Label("Parent entity");
		gui::Button* button = new gui::Button(util::str2wstr_utf8(currentParent ? currentParent->name : NOT_SET), glm::vec4(10.f), 
		[=, &panel, &entity](gui::GUI*) {
			if (showUnsaved()) return;
			createEntitiesList(true, 5, panel.calcPos().x + panel.getSize().x, [=, &entity](std::string string) {
				const std::string name = entity.name;
				if (string == entity.name) string.clear();
				saveEntity(entity, currentPack.folder, currentParent, string);
				removePanels(1);
				initialize();
				createEntitiesList();
				createEntityEditorPanel(*content->entities.getDefs().at(name));
			});
		});
		panel << button;

		panel << (button = new gui::Button(L"Body type: " + util::str2wstr_utf8(to_string(entity.bodyType)), glm::vec4(10.f), gui::onaction()));
		button->listenAction([button, &entity](gui::GUI*) {
			incrementEnumClass(entity.bodyType, 1);
			if (entity.bodyType > BodyType::DYNAMIC) entity.bodyType = BodyType::STATIC;
			button->setText(L"Body type: " + util::str2wstr_utf8(to_string(entity.bodyType)));
		});

		auto skeletonName = [this](const std::string& name) {
			if (content->getSkeleton(name)) return name;
			return NOT_SET;
		};

		panel << new gui::Label("Skeleton");
		panel << (button = new gui::Button(util::str2wstr_utf8(skeletonName(entity.skeletonName)), glm::vec4(10.f), gui::onaction()));
		button->listenAction([this, button, &entity, &panel, skeletonName](gui::GUI*) {
			createSkeletonList(true, 5, panel.calcPos().x + panel.getSize().x, [this, button, &entity, &panel, skeletonName](const std::string& string) {
				entity.skeletonName = skeletonName(string) == NOT_SET ? (content->getSkeleton(entity.name) ? "" : entity.name) : string;
				button->setText(util::str2wstr_utf8(skeletonName(entity.skeletonName)));
				preview->setSkeleton(content->getSkeleton(entity.skeletonName));
				removePanel(5);
			});
		});
		
		panel << new gui::Label("Hitbox (0 - 100)");
		panel << createVectorPanel(entity.hitbox, glm::vec3(0.f), glm::vec3(100.f), panel.getSize().x, InputMode::TEXTBOX, runnable());

		panel << new gui::Button(L"Save", glm::vec4(10.f), [=, &entity, &backupData](gui::GUI*) {
			const std::string parentName = currentParent ? currentParent->name : "";
			backupData.serialized = stringify(entity, currentParent, parentName);
			saveEntity(entity, currentPack.folder, currentParent, parentName);
		});

		bool isParent = false;
		std::wstring parentOf = L"Action impossible: Parent of ";
		for (const auto& [name, data] : entityList) {
			if (data.parent == entity.name) {
				parentOf.append(util::str2wstr_utf8(name) + L'\n');
				isParent = true;
			}
		}

		panel << (button = new gui::Button(L"Rename", glm::vec4(10.f), gui::onaction()));
		if (isParent) button->setTooltip(parentOf);
		else button->listenAction([this, actualName](gui::GUI*) {
			createDefActionPanel(ContentAction::RENAME, ContentType::ENTITY, actualName);
		});

		panel << (button = new gui::Button(L"Delete", glm::vec4(10.f), gui::onaction()));
		if (isParent) button->setTooltip(parentOf);
		else button->listenAction([this, actualName](gui::GUI*) {
			createDefActionPanel(ContentAction::DELETE, ContentType::ENTITY, actualName);
		});

		createAdditionalEntityEditorPanel(entity);

		preview->setSkeleton(content->getSkeleton(entity.skeletonName));

		return std::ref(panel);
	}, 2);
}

void WorkShopScreen::createEntityComponentsEditor(gui::Panel& panel, EntityDef& entity) {
	panel.clear();
	panel.setOrientation(gui::Orientation::horizontal);
	panel.setPadding(glm::vec4(0.f));
	panel.setMargin(glm::vec4(0.f));

	gui::Panel& editorPanel = *new gui::Panel(glm::vec2(settings.entityEditorWidth));
	editorPanel.setMargin(glm::vec4(0.f));
	editorPanel.setColor(glm::vec4(0.f));
	editorPanel.setSizeFunc([&editorPanel, &panel]() {
		return glm::vec2(editorPanel.getSize().x, panel.getSize().y);
	});
	panel << editorPanel;

	gui::Panel& previewPanel = createSkeletonPreview();
	previewPanel.setMargin(glm::vec4(0.f));
	previewPanel.setColor(glm::vec4(0.f));
	panel << previewPanel;
	previewPanel.act(1.f);

	for (size_t i = 0; i < entity.components.size(); i++) {
		gui::Container& container = *new gui::Container(glm::vec2(panel.getSize().x));
		container.setScrollable(false);

		gui::Button& deleteButton = *new gui::Button(std::make_shared<gui::Image>("gui/delete_icon"));
		deleteButton.listenAction([=, &panel, &entity](gui::GUI*) {
			entity.components.erase(entity.components.begin() + i);
			createEntityComponentsEditor(panel, entity);
		});

		const glm::vec2 size(editorPanel.getSize().x - deleteButton.getSize().x, deleteButton.getSize().y + 4);

		createTextBox(container, entity.components[i]);
		container.setSize(glm::vec2(container.getSize().x, (container.getNodes().front()->getSize().y + 4) * container.getNodes().size()));

		float posY = 0.f;
		for (size_t i = 0; i < container.getNodes().size(); i++) {
			auto& node = container.getNodes()[i];
			node->setSize(size);
			node->setPos(glm::vec2(node->getPos().x, posY));
			deleteButton.setPos(glm::vec2(size.x, posY / 2.f));
			posY += node->getSize().y + node->getMargin().y * 2;
		}
		container << deleteButton;

		editorPanel << container;
	}

	editorPanel << new gui::Button(L"Add component", glm::vec4(10.f), [=, &panel, &entity](gui::GUI*) {
		entity.components.emplace_back("");
		createEntityComponentsEditor(panel, entity);
	});
}

static size_t getSensorsCount(const EntityDef& entity, SensorType type){
	switch (type) {
		case SensorType::AABB: return entity.boxSensors.size();
		case SensorType::RADIUS: return entity.radialSensors.size();
		default: return 0;
	}
}

void WorkShopScreen::createEntitySensorsEditor(gui::Panel& panel, EntityDef& entity, SensorType type) {
	panel.clear();
	panel.setOrientation(gui::Orientation::horizontal);
	panel.setPadding(glm::vec4(0.f));
	panel.setMargin(glm::vec4(0.f));

	gui::Panel& editorPanel = *new gui::Panel(glm::vec2(settings.entityEditorWidth));
	editorPanel.setMargin(glm::vec4(0.f));
	editorPanel.setColor(glm::vec4(0.f));
	editorPanel.setSizeFunc([&editorPanel, &panel]() {
		return glm::vec2(editorPanel.getSize().x, panel.getSize().y);
	});
	panel << editorPanel;

	gui::Panel& previewPanel = createSkeletonPreview();
	previewPanel.setMargin(glm::vec4(0.f));
	previewPanel.setColor(glm::vec4(0.f));
	panel << previewPanel;
	previewPanel.act(1.f);

	const std::wstring sensorsNames[] = { L"AABB", L"Radius" };
	editorPanel << new gui::Button(L"Sensor type: " + sensorsNames[static_cast<int>(type)], glm::vec4(10.f), [=, &panel, &entity](gui::GUI*) mutable {
		incrementEnumClass(type, 1);
		if (type > SensorType::RADIUS) type = SensorType::AABB;
		createEntitySensorsEditor(panel, entity, type);
	});

	const size_t sensorsCount = getSensorsCount(entity, type);

	for (size_t i = 0; i < sensorsCount; i++) {
		gui::Container& container = *new gui::Container(glm::vec2(editorPanel.getSize().x));
		container.setScrollable(false);

		const size_t sensorIndex = type == SensorType::RADIUS ? entity.radialSensors[i].first : entity.boxSensors[i].first;
		editorPanel << new gui::Label("Index: " + std::to_string(sensorIndex));

		gui::Button& deleteButton = *new gui::Button(std::make_shared<gui::Image>("gui/delete_icon"));
		deleteButton.listenAction([=, &panel, &entity](gui::GUI*) {
			for (auto& sensor : entity.boxSensors) {
				if (sensor.first > sensorIndex) sensor.first--;
			}
			for (auto& sensor : entity.radialSensors) {
				if (sensor.first > sensorIndex) sensor.first--;
			}
			switch (type) {
				case SensorType::AABB:
					entity.boxSensors.erase(entity.boxSensors.begin() + i);
					break;
				case SensorType::RADIUS:
					entity.radialSensors.erase(entity.radialSensors.begin() + i);
					break;
			}
			createEntitySensorsEditor(panel, entity, type);
		});

		const glm::vec2 size(editorPanel.getSize().x - deleteButton.getSize().x, deleteButton.getSize().y + 4);

		if (type == SensorType::AABB) {
			AABB& aabb = entity.boxSensors[i].second;
			container << createVectorPanel(aabb.a, glm::vec3(-100.f), glm::vec3(100.f), size.x, InputMode::TEXTBOX);
			container << createVectorPanel(aabb.b, glm::vec3(-100.f), glm::vec3(100.f), size.x, InputMode::TEXTBOX);
		}
		else if (type == SensorType::RADIUS) container << createNumTextBox(entity.radialSensors[i].second, L"Radius", 3, 0.f, 100.f);
		container.setSize(glm::vec2(container.getSize().x, (container.getNodes().front()->getSize().y + 4) * container.getNodes().size()));

		float posY = 0.f;
		for (size_t i = 0; i < container.getNodes().size(); i++) {
			auto& node = container.getNodes()[i];
			node->setSize(size);
			node->setPos(glm::vec2(node->getPos().x, posY));
			deleteButton.setPos(glm::vec2(size.x, posY / 2.f));
			posY += node->getSize().y + node->getMargin().y * 2;
		}
		container << deleteButton;

		editorPanel << container;
	}

	editorPanel << new gui::Button(L"Add sensor", glm::vec4(10.f), [=, &panel, &entity](gui::GUI*) {
		const size_t allSensorsCount = entity.boxSensors.size() + entity.radialSensors.size();
		switch (type) {
			case SensorType::AABB:
				entity.boxSensors.emplace_back(allSensorsCount, AABB());
				break;
			case SensorType::RADIUS:
				entity.radialSensors.emplace_back(allSensorsCount, 1.f);
				break;
		}
		createEntitySensorsEditor(panel, entity, type);
	});
}

void WorkShopScreen::createAdditionalEntityEditorPanel(EntityDef& entity) {
	createPanel([this, &entity]() {
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
		createEntitySensorsEditor(*panel, entity, SensorType::AABB);
		tabsContainer.addTab("sensors", L"Sensors", panel);

		panel = new gui::Panel(glm::vec2());
		panel->setColor(glm::vec4(0.f));
		createEntityComponentsEditor(*panel, entity);
		tabsContainer.addTab("components", L"Components", panel);

		return std::ref(mainPanel);
	}, 3);
}