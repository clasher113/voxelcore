#include "../WorkshopScreen.hpp"

#include "content/Content.hpp"
#include "../IncludeCommons.hpp"
#include "objects/EntityDef.hpp"
#include "../WorkshopSerializer.hpp"
#include "graphics/core/Texture.hpp"

using namespace workshop;

constexpr unsigned int MODE_COMPONENTS = 0;
constexpr unsigned int MODE_AABB_SENSORS = 1;
constexpr unsigned int MODE_RADIAL_SENSORS = 2;

void WorkShopScreen::createEntityEditorPanel(EntityDef& entity) {
	createPanel([this, &entity]() {
		const std::string actualName(getDefName(entity.name));

		gui::Panel& panel = *new gui::Panel(glm::vec2(settings.itemEditorWidth));

		panel += new gui::Label(actualName);

		createFullCheckBox(panel, L"Blocking", entity.blocking, L"Does entity prevent blocks setup");
		gui::Panel& savingPanel = *new gui::Panel(panel.getSize(), glm::vec4(0.f));
		savingPanel.setColor(glm::vec4(0.f));
		auto processSavingChange = [&entity, &savingPanel, &panel]() {
			removeRemovable(savingPanel);
			if (entity.save.enabled) {
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
		panel += savingPanel;
		processSavingChange();

		auto parentName = [](const EntityDef* const parent) {
			return parent ? parent->name : NOT_SET;
		};

		BackupData& backupData = entityList[actualName];
		const EntityDef* currentParent = content->entities.find(backupData.currentParent);
		const EntityDef* newParent = content->entities.find(backupData.newParent);

		panel += new gui::Label("Parent entity");
		gui::Button* button = new gui::Button(util::str2wstr_utf8(parentName(newParent)), glm::vec4(10.f), gui::onaction());
		button->listenAction([this, button, parentName,&panel, &entity, &backupData](gui::GUI*) {
			createEntitiesList(true, 5, panel.calcPos().x + panel.getSize().x, [=, &backupData, &button, &entity](const std::string& string) {
				const EntityDef* parent = content->entities.find(string);
				if (parent && parent->name == entity.name) parent = nullptr;
				backupData.newParent = parent ? string : "";
				button->setText(util::str2wstr_utf8(parentName(parent)));
				removePanels(5);
			});
		});
		panel += button;

		button = new gui::Button(L"Body type: " + util::str2wstr_utf8(to_string(entity.bodyType)), glm::vec4(10.f), gui::onaction());
		button->listenAction([button, &entity](gui::GUI*) {
			entity.bodyType = incrementEnumClass(entity.bodyType, 1);
			if (entity.bodyType > BodyType::DYNAMIC) entity.bodyType = BodyType::STATIC;
			button->setText(L"Body type: " + util::str2wstr_utf8(to_string(entity.bodyType)));
		});
		panel += button;
		
		panel += new gui::Label("Hitbox (0 - 100)");
		panel += createVectorPanel(entity.hitbox, glm::vec3(0.f), glm::vec3(100.f), panel.getSize().x, 1, runnable());

		panel += new gui::Button(L"Save", glm::vec4(10.f), [this, &entity, actualName, currentParent, &backupData](gui::GUI*) {
			backupData.string = stringify(toJson(entity, actualName, currentParent, backupData.newParent), false);
			saveEntity(entity, currentPack.folder, actualName, currentParent, backupData.newParent);
		});
		panel += new gui::Button(L"Rename", glm::vec4(10.f), [this, actualName](gui::GUI*) {
			createDefActionPanel(ContentAction::RENAME, ContentType::ENTITY, actualName);
		});
		panel += new gui::Button(L"Delete", glm::vec4(10.f), [this, actualName](gui::GUI*) {
			createDefActionPanel(ContentAction::DELETE, ContentType::ENTITY, actualName);
		});

		createAdditionalEntityEditorPanel(entity, MODE_COMPONENTS);

		return std::ref(panel);
	}, 2);
}

static void createList(gui::Panel& panel, gui::Panel& parentPanel, EntityDef& entity, unsigned int mode, Assets* assets) {
	panel.clear();
	panel.cropToContent();
	parentPanel.refresh();

	size_t arrSize = 0;
	if (mode == MODE_COMPONENTS) arrSize = entity.components.size();
	else if (mode == MODE_AABB_SENSORS) arrSize = entity.boxSensors.size();
	else if (mode == MODE_RADIAL_SENSORS) arrSize = entity.radialSensors.size();

	for (size_t i = 0; i < arrSize; i++) {
		gui::Container& container = *new gui::Container(glm::vec2(panel.getSize().x));
		container.setScrollable(false);

		gui::Image& image = *new gui::Image(assets->get<Texture>("gui/delete_icon"));
		gui::Container& imageContainer = *new gui::Container(image.getSize());

		const glm::vec2 size(panel.getSize().x - image.getSize().x, image.getSize().y);
		if (mode != MODE_COMPONENTS) panel += new gui::Label(L"Index: " + std::to_wstring(mode == MODE_RADIAL_SENSORS ? entity.radialSensors[i].first : entity.boxSensors[i].first));
		if (mode == MODE_COMPONENTS) createTextBox(container, entity.components[i]);
		else if (mode == MODE_AABB_SENSORS) {
			AABB& aabb = entity.boxSensors[i].second;
			container += createVectorPanel(aabb.a, glm::vec3(-100.f), glm::vec3(100.f), size.x + 4.f, 1, runnable());
			container += createVectorPanel(aabb.b, glm::vec3(-100.f), glm::vec3(100.f), size.x + 4.f, 1, runnable());
		}
		else if (mode == MODE_RADIAL_SENSORS) container += createNumTextBox(entity.radialSensors[i].second, L"Radius", 0.f, 100.f);
		imageContainer.listenAction([i, &panel, &parentPanel, mode, assets, &entity](gui::GUI*) {
			if (mode != MODE_COMPONENTS) {
				const size_t removingSensorIndex = mode == MODE_RADIAL_SENSORS ? entity.radialSensors[i].first : entity.boxSensors[i].first;
				for (auto& sensor : entity.boxSensors) {
					if (sensor.first > removingSensorIndex) sensor.first--;
				}
				for (auto& sensor : entity.radialSensors) {
					if (sensor.first > removingSensorIndex) sensor.first--;
				}
			}
			if (mode == MODE_AABB_SENSORS) entity.boxSensors.erase(entity.boxSensors.begin() + i);
			else if (mode == MODE_RADIAL_SENSORS) entity.radialSensors.erase(entity.radialSensors.begin() + i);
			else if (mode == MODE_COMPONENTS) entity.components.erase(entity.components.begin() + i);
			createList(panel, parentPanel, entity, mode, assets);
		});

		container.setSize(glm::vec2(container.getSize().x, container.getNodes().front()->getSize().y * container.getNodes().size() - 1));
		container += imageContainer;

		float posY = 0.f;
		for (size_t i = 0; i < container.getNodes().size() - 1; i++) {
			auto& node = container.getNodes()[i];
			node->setSize(size);
			node->setPos(glm::vec2(node->getPos().x, posY));
			imageContainer.setPos(glm::vec2(size.x, posY / 2.f));
			posY += node->getSize().y + node->getMargin().y * 2;
		}
		imageContainer += image;

		panel += container;
	}
}

void WorkShopScreen::createAdditionalEntityEditorPanel(EntityDef& entity, unsigned int mode) {
	createPanel([this, &entity, mode]() {
		gui::Panel& panel = *new gui::Panel(glm::vec2(settings.itemEditorWidth));
		
		panel += new gui::Button(std::wstring(L"Mode: ") + (mode == MODE_COMPONENTS ? L"Components" : L"Sensors"), glm::vec4(10.f), [this, &entity, mode](gui::GUI*) {
			createAdditionalEntityEditorPanel(entity, mode == MODE_COMPONENTS ? MODE_AABB_SENSORS : MODE_COMPONENTS);
		});
		if (mode != MODE_COMPONENTS){
			panel += new gui::Button(std::wstring(L"Sensor type: ") + (mode == MODE_AABB_SENSORS ? L"AABB" : L"Radial"), glm::vec4(10.f), [this, &entity, mode](gui::GUI*) {
				createAdditionalEntityEditorPanel(entity, mode == MODE_AABB_SENSORS ? MODE_RADIAL_SENSORS : MODE_AABB_SENSORS);
			});
		}

		gui::Panel& listPanel = *new gui::Panel(glm::vec2(0.f));
		listPanel.setColor(glm::vec4(0.f));
		panel += listPanel;

		createList(listPanel, panel, entity, mode, assets);

		panel += new gui::Button(std::wstring(L"Add ") + (mode == MODE_COMPONENTS ? L"component" : L"sensor"), glm::vec4(10.f), [this, &listPanel, &panel, &entity, mode](gui::GUI*) {
			const size_t sensorsCount = entity.boxSensors.size() + entity.radialSensors.size();
			if (mode == MODE_COMPONENTS) entity.components.emplace_back("");
			else if (mode == MODE_AABB_SENSORS) entity.boxSensors.emplace_back(sensorsCount, AABB());
			else if (mode == MODE_RADIAL_SENSORS) entity.radialSensors.emplace_back(sensorsCount, 1.f);
			createList(listPanel, panel, entity, mode, assets);
		});

		return std::ref(panel);
	}, 3);
}