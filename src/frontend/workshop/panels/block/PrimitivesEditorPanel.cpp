#include "frontend/workshop/WorkshopScreen.hpp"

#include "graphics/ui/elements/Panel.hpp"
#include "graphics/ui/elements/Button.hpp"
#include "graphics/ui/elements/Label.hpp"
#include "graphics/ui/elements/Textbox.hpp"
#include "frontend/workshop/gui_elements/BasicElements.hpp"
#include "frontend/workshop/WorkshopPreview.hpp"
#include "voxels/Block.hpp"
#include "core_defs.hpp"

using namespace workshop;

static InputMode inputMode = InputMode::SLIDER;
static std::vector<glm::vec3> points;
static std::vector<std::string> textures;

static size_t getPrimitivesCount(const Block& block, PrimitiveType type) {
	switch (type) {
		case PrimitiveType::AABB:
			if (block.customModelRaw.has(AABB_STR)) return block.customModelRaw[AABB_STR].size();
			return 0;
		case PrimitiveType::TETRAGON:
			if (block.customModelRaw.has(TETRAGON_STR)) return block.customModelRaw[TETRAGON_STR].size();
			return 0;
		case PrimitiveType::HITBOX:
			return block.hitboxes.size();
	}
	return 0;
}

static void fetchModelData(const Block& block, PrimitiveType type) {
	points.clear();
	textures.clear();

	if (type == PrimitiveType::AABB) {
		if (!block.customModelRaw.has(AABB_STR)) return;
		const dv::value& aabbs = block.customModelRaw[AABB_STR];
		for (const dv::value& aabb : aabbs) {
			points.emplace_back(exportAABB(aabb).a);
			points.emplace_back(exportAABB(aabb).b);
			if (aabb.size() == 7) {
				for (uint j = 6; j < 12; j++) {
					textures.emplace_back(aabb[6].asString());
				}
			}
			else if (aabb.size() == 12) {
				for (uint j = 6; j < 12; j++) {
					textures.emplace_back(aabb[j].asString());
				}
			}
		}

	}
	else if (type == PrimitiveType::TETRAGON) {
		if (!block.customModelRaw.has(TETRAGON_STR)) return;
		const dv::value& tetragons = block.customModelRaw[TETRAGON_STR];
		for (const dv::value& tetragon : tetragons) {
			for (const glm::vec3& point : exportTetragon(tetragon)) {
				points.emplace_back(point);
			}
			textures.emplace_back(tetragon[9].asString());
		}
	}
}

static void applyModelData(Block& block, PrimitiveType type) {
	if (points.empty()) return;
	const std::string currentPrimitiveId = type == PrimitiveType::AABB ? AABB_STR : TETRAGON_STR;
	if (!block.customModelRaw.has(currentPrimitiveId))
		block.customModelRaw[currentPrimitiveId] = dv::list();
	dv::value& primitives = block.customModelRaw[currentPrimitiveId];
	while (!primitives.empty()) {
		primitives.erase(0);
	}

	if (type == PrimitiveType::AABB) {
		for (size_t i = 0; i < points.size() / 2; i++) {
			dv::value& aabb = primitives.list();
			putVec3(aabb, points[i * 2]);
			putVec3(aabb, points[i * 2 + 1]);
			for (size_t j = i * 6; j < i * 6 + 6; j++) {
				aabb.add(textures[j]);
			}
		}
	}
	else if (type == PrimitiveType::TETRAGON) {
		for (size_t i = 0; i < points.size() / 4; i++) {
			dv::value& tetragon = primitives.list();
			putVec3(tetragon, points[i * 4]);
			putVec3(tetragon, points[i * 4 + 1] - points[i * 4]);
			putVec3(tetragon, points[i * 4 + 3] - points[i * 4]);
			tetragon.add(textures[i]);
		}
	}
}

void WorkShopScreen::createPrimitiveEditor(gui::Panel& panel, Block& block, size_t index, PrimitiveType type) {
	panel.clear();
	panel.setOrientation(gui::Orientation::horizontal);
	panel.setPadding(glm::vec4(0.f));
	panel.setMargin(glm::vec4(0.f));

	gui::Panel& editorPanel = *new gui::Panel(glm::vec2(settings.customModelEditorWidth));
	editorPanel.setMargin(glm::vec4(0.f));
	editorPanel.setColor(glm::vec4(0.f));
	editorPanel.setSizeFunc([&editorPanel, &panel]() {
		return glm::vec2(editorPanel.getSize().x, panel.getSize().y);
	});
	panel << editorPanel;

	gui::Panel& previewPanel = createBlockPreview(panel, block, type);
	previewPanel.setMargin(glm::vec4(0.f));
	previewPanel.setColor(glm::vec4(0.f));
	panel << previewPanel;
	previewPanel.act(1.f);

	const std::wstring currentPrimitiveName(getPrimitiveName(type));
	const std::string currentPrimitiveId = type == PrimitiveType::AABB ? AABB_STR : TETRAGON_STR;
	if (!block.customModelRaw.has(currentPrimitiveId))
		block.customModelRaw[currentPrimitiveId] = dv::list();
	dv::value& currentPrimitiveList = block.customModelRaw[currentPrimitiveId];

	if (type != PrimitiveType::HITBOX) {
		editorPanel << new gui::Button(L"Import model", glm::vec4(10.f), [this, &editorPanel, &block](gui::GUI*) {
			createBlockConverterPanel(block, editorPanel.calcPos().x + editorPanel.getSize().x);
		});
		editorPanel << new gui::Button(L"Primitive: " + currentPrimitiveName, glm::vec4(10.f), [=, &panel, &block](gui::GUI*) mutable {
			modelPrimitive = incrementEnumClass(type, 1);
			if (modelPrimitive > PrimitiveType::TETRAGON) modelPrimitive = PrimitiveType::AABB;
			createPrimitiveEditor(panel, block, 0, modelPrimitive);
		});
	}
	const size_t primitivesCount = getPrimitivesCount(block, type);
	editorPanel << new gui::Label(currentPrimitiveName + L": " + std::to_wstring(primitivesCount == 0 ? 0 : index + 1) + L"/" + std::to_wstring(primitivesCount));

	editorPanel << new gui::Button(L"Add new", glm::vec4(10.f), [=, &panel, &block, &currentPrimitiveList](gui::GUI*) {
		if (type == PrimitiveType::HITBOX) {
			block.hitboxes.emplace_back();
		}
		else {
			const dv::value template_ = type == PrimitiveType::AABB ?
				dv::list({ 0.f, 0.f, 0.f, 1.f, 1.f, 1.f, TEXTURE_NOTFOUND, TEXTURE_NOTFOUND, TEXTURE_NOTFOUND, TEXTURE_NOTFOUND, TEXTURE_NOTFOUND, TEXTURE_NOTFOUND }) :
				dv::list({ 0.f, 0.f, 0.5f, 1.f, 0.f, 0.f, 0.f, 1.f, 0.f, TEXTURE_NOTFOUND });

			currentPrimitiveList.add(template_);
		}

		createPrimitiveEditor(panel, block, getPrimitivesCount(block, type) - 1, type);
		preview->updateCache();
	});

	if (getPrimitivesCount(block, type) == 0) return;

	fetchModelData(block, type);

	editorPanel << new gui::Button(L"Copy current", glm::vec4(10.f), [=, &panel, &block, &currentPrimitiveList](gui::GUI*) {
		if (type == PrimitiveType::HITBOX) {
			block.hitboxes.emplace_back(block.hitboxes[index]);
		}
		else {
			currentPrimitiveList.add(currentPrimitiveList[index]);
			preview->updateCache();
		}
		createPrimitiveEditor(panel, block, getPrimitivesCount(block, type) - 1, type);
	});
	editorPanel << new gui::Button(L"Remove current", glm::vec4(10.f), [=, &panel, &block, &currentPrimitiveList](gui::GUI*) {
		if (type == PrimitiveType::HITBOX) {
			block.hitboxes.erase(block.hitboxes.begin() + index);
		}
		else {
			currentPrimitiveList.erase(index);
			preview->updateCache();
		}
		createPrimitiveEditor(panel, block, std::min(getPrimitivesCount(block, type) - 1, index), type);
	});
	editorPanel << new gui::Button(L"Previous", glm::vec4(10.f), [=, &panel, &block](gui::GUI*) {
		if (index != 0) {
			createPrimitiveEditor(panel, block, index - 1, type);
		}
	});
	editorPanel << new gui::Button(L"Next", glm::vec4(10.f), [=, &panel, &block](gui::GUI*) {
		const size_t next = index + 1;
		if (next < primitivesCount) {
			createPrimitiveEditor(panel, block, next, type);
		}
	});

	auto createInputModeButton = [](const std::function<void()>& callback) {
		const std::wstring inputModes[] = { L"Slider", L"InputBox" };
		gui::Button* button = new gui::Button(L"Input mode: " + inputModes[static_cast<int>(inputMode)], glm::vec4(10.f), gui::onaction());
		button->listenAction([button, inputModes, callback](gui::GUI*) {
			incrementEnumClass(inputMode, 1);
			if (inputMode > InputMode::TEXTBOX) inputMode = InputMode::SLIDER;
			button->setText(L"Input mode: " + inputModes[static_cast<int>(inputMode)]);
			callback();
		});
		return button;
	};

	gui::Panel& inputPanel = *new gui::Panel(glm::vec2(editorPanel.getSize().x));
	inputPanel.setColor(glm::vec4(0.f));
	editorPanel << inputPanel;

	std::function<void()> createInput;
	std::string* textures_ptr = nullptr;
	if (type == PrimitiveType::HITBOX) {
		AABB& aabb = block.hitboxes[index];

		preview->setCurrentAABB(aabb.a, aabb.b, type);
		createInput = [=, &inputPanel, &editorPanel, &block, &aabb]() {
			removeRemovable(inputPanel);
			inputPanel << markRemovable(createVectorPanel(aabb.a, glm::vec3(settings.customModelRange.x),
				glm::max(glm::vec3(settings.customModelRange.y), glm::vec3(block.size)), editorPanel.getSize().x, inputMode, [=, &aabb]() {
				preview->updateMesh();
				preview->setCurrentAABB(aabb.a, aabb.b, type);
			}
			));
			inputPanel << markRemovable(createVectorPanel(aabb.b, glm::vec3(settings.customModelRange.x),
				glm::max(glm::vec3(settings.customModelRange.y), glm::vec3(block.size)), editorPanel.getSize().x, inputMode, [=, &aabb]() {
				preview->updateMesh();
				preview->setCurrentAABB(aabb.a, aabb.b, type);
			}
			));
		};
	}
	else if (type == PrimitiveType::TETRAGON) {
		preview->setCurrentTetragon(points.data() + index * 4);

		createInput = [=, &block, &inputPanel, &editorPanel]() {
			removeRemovable(inputPanel);
			for (size_t i = 0; i < 4; i++) {
				if (i == 2) continue;
				glm::vec3* vec = &points[index * 4];
				inputPanel << markRemovable(createVectorPanel(vec[i], glm::vec3(settings.customModelRange.x),
					glm::max(glm::vec3(settings.customModelRange.y), glm::vec3(block.size)), editorPanel.getSize().x, inputMode, [=, &block]() {
					applyModelData(block, type);
					const glm::vec3& p1 = vec[0];
					const glm::vec3 xw = vec[1] - p1;
					const glm::vec3 yh = vec[3] - p1;
					vec[2] = p1 + xw + yh;
					preview->setCurrentTetragon(points.data() + index * 4);
					preview->updateCache();
				}
				));
			}
		};
		textures_ptr = textures.data() + index;
	}
	else if (type == PrimitiveType::AABB) {
		preview->setCurrentAABB(points[index * 2], points[index * 2 + 1], type);

		createInput = [=, &inputPanel, &editorPanel, &block, &currentPrimitiveList]() {
			removeRemovable(inputPanel);

			inputPanel << markRemovable(createVectorPanel(points[index * 2], glm::vec3(settings.customModelRange.x),
				glm::max(glm::vec3(settings.customModelRange.y), glm::vec3(block.size)), editorPanel.getSize().x, inputMode, [=, &block]() {
				applyModelData(block, type);
				preview->setCurrentAABB(points[index * 2], points[index * 2 + 1], type);
				preview->updateCache();
			}));
			inputPanel << markRemovable(createVectorPanel(points[index * 2 + 1], glm::vec3(settings.customModelRange.x),
				glm::max(glm::vec3(settings.customModelRange.y), glm::vec3(block.size)), editorPanel.getSize().x, inputMode, [=, &block]() {
				applyModelData(block, type);
				preview->setCurrentAABB(points[index * 2], points[index * 2 + 1], type);
				preview->updateCache();
			}));
		};
		textures_ptr = textures.data() + index * 6;
	}
	createInput();
	editorPanel << createInputModeButton(createInput);
	if (type != PrimitiveType::HITBOX) {
		gui::Panel& texturePanel = *new gui::Panel(glm::vec2(editorPanel.getSize().x, 35.f));
		createTexturesPanel(texturePanel, 35.f, textures_ptr, (type == PrimitiveType::AABB ? BlockModel::aabb : BlockModel::custom), [=, &block]() {
			applyModelData(block, type);
		}
		);
		editorPanel << texturePanel;
	}
	if (type == PrimitiveType::AABB) {
		editorPanel << new gui::Button(L"Convert to tetragons", glm::vec4(10.f), [=, &panel, &block, &currentPrimitiveList](gui::GUI*) {
			std::vector<glm::vec3> tetragons = aabb2tetragons(points[index * 2], points[index * 2 + 1] + points[index * 2]);
			std::vector<std::string> tex(textures.data() + index * 6, textures.data() + index * 6 + 6);
			currentPrimitiveList.erase(index);
			fetchModelData(block, PrimitiveType::TETRAGON);
			points.insert(points.end(), tetragons.begin(), tetragons.end());
			textures.insert(textures.end(), tex.begin(), tex.end());
			applyModelData(block, PrimitiveType::TETRAGON);
			preview->updateCache();
			createPrimitiveEditor(panel, block, getPrimitivesCount(block, PrimitiveType::TETRAGON) - 1, PrimitiveType::TETRAGON);
		});
	}

	editorPanel << new gui::Label("Go to:");
	gui::TextBox* goTo = new gui::TextBox(L"Primitive index");
	goTo->setTextConsumer([=, &panel, &block](const std::wstring&) {
		try {
			int result = stoi(goTo->getInput());
			if (result >= 1 && result <= primitivesCount) {
				createPrimitiveEditor(panel, block, result - 1, type);
			}
			else goTo->setText(L"");
		}
		catch (const std::exception&) {
			goTo->setText(L"");
		}
	});
	editorPanel << goTo;
}