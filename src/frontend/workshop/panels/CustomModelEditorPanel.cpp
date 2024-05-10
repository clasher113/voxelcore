#include "../WorkshopScreen.hpp"

#include "../IncludeCommons.hpp"
#include "../WorkshopPreview.hpp"

void workshop::WorkShopScreen::createCustomModelEditor(Block& block, size_t index, PrimitiveType type) {
	createPanel([this, &block, index, type]() mutable {
		auto panel = std::make_shared<gui::Panel>(glm::vec2(200));
		createBlockPreview(4, type);

		std::vector<AABB>& aabbArr = (type == PrimitiveType::AABB ?block.modelBoxes :block.hitboxes);
		const std::wstring primitives[] = { L"AABB", L"Tetragon", L"Hitbox" };
		const std::wstring editorModes[] = { L"Custom model", L"Custom model", L"Hitbox" };
		const glm::vec3 tetragonTemplate[] = { glm::vec3(0.f, 0.f, 0.5f), glm::vec3(1.f, 0.f, 0.5f), glm::vec3(1.f, 1.f, 0.5f), glm::vec3(0.f, 1.f, 0.5f) };
		const std::wstring currentPrimitiveName(primitives[static_cast<unsigned int>(type)]);
		if (block.model == BlockModel::custom) {
			panel->add(std::make_shared<gui::Button>(L"Mode: " + currentPrimitiveName, glm::vec4(10.f), [this, &block, type](gui::GUI*) {
				if (type == PrimitiveType::HITBOX) createCustomModelEditor(block, 0, PrimitiveType::AABB);
				else createCustomModelEditor(block, 0, PrimitiveType::HITBOX);
			}));
		}
		if (type != PrimitiveType::HITBOX) {
			panel->add(std::make_shared<gui::Button>(L"Primitive: " + currentPrimitiveName, glm::vec4(10.f), [this, &block, type](gui::GUI*) {
				if (type == PrimitiveType::AABB) createCustomModelEditor(block, 0, PrimitiveType::TETRAGON);
				else if (type == PrimitiveType::TETRAGON) createCustomModelEditor(block, 0, PrimitiveType::AABB);

			}));
		}
		size_t size = (type == PrimitiveType::TETRAGON ?block.modelExtraPoints.size() / 4 : aabbArr.size());
		panel->add(std::make_shared<gui::Label>(currentPrimitiveName + L": " + std::to_wstring(size == 0 ? 0 : index + 1) + L"/" + std::to_wstring(size)));

		panel->add(std::make_shared<gui::Button>(L"Add new", glm::vec4(10.f), [this, &block, type, &aabbArr, tetragonTemplate](gui::GUI*) {
			size_t index = 0;
			if (type == PrimitiveType::TETRAGON) {
				block.modelTextures.emplace_back("notfound");
				for (size_t i = 0; i < 4; i++) {
					block.modelExtraPoints.emplace_back(tetragonTemplate[i]);
				}
				index =block.modelExtraPoints.size() / 4;
			}
			else {
				if (type == PrimitiveType::AABB)
					block.modelTextures.insert(block.modelTextures.begin() + aabbArr.size() * 6, 6, "notfound");
				aabbArr.emplace_back(AABB());
				index = aabbArr.size();
			}
			createCustomModelEditor(block, index - 1, type);
			if (type != PrimitiveType::HITBOX) preview->updateCache();
		}));

		if (type == PrimitiveType::AABB &&block.modelBoxes.empty() ||
			type == PrimitiveType::TETRAGON &&block.modelExtraPoints.empty() ||
			type == PrimitiveType::HITBOX &&block.hitboxes.empty()) return panel;
		panel->add(std::make_shared<gui::Button>(L"Copy current", glm::vec4(10.f), [this, &block, index, type, &aabbArr](gui::GUI*) {
			if (type == PrimitiveType::TETRAGON) {
				block.modelTextures.emplace_back(*(block.modelTextures.begin() +block.modelBoxes.size() * 6 + index));
				for (size_t i = 0; i < 4; i++) {
					block.modelExtraPoints.emplace_back(*(block.modelExtraPoints.begin() + index * 4 + i));
				}
			}
			else {
				if (type == PrimitiveType::AABB) {
					for (size_t i = 0; i < 6; i++) {
						block.modelTextures.emplace(block.modelTextures.begin() +block.modelBoxes.size() * 6 + i, *(block.modelTextures.begin() + index * 6 + i));
					}
				}
				aabbArr.emplace_back(aabbArr[index]);
			}
			createCustomModelEditor(block, (type == PrimitiveType::TETRAGON ?block.modelExtraPoints.size() / 4 - 1 : aabbArr.size() - 1), type);
			if (type != PrimitiveType::HITBOX) preview->updateCache();
		}));
		panel->add(std::make_shared<gui::Button>(L"Remove current", glm::vec4(10.f), [this, &block, index, type, &aabbArr](gui::GUI*) {
			if (type == PrimitiveType::TETRAGON) {
				auto it =block.modelExtraPoints.begin() + index * 4;
				block.modelExtraPoints.erase(it, it + 4);
				block.modelTextures.erase(block.modelTextures.begin() +block.modelBoxes.size() * 6 + index);
				createCustomModelEditor(block, std::min(block.modelExtraPoints.size() / 4 - 1, index), type);
			}
			else {
				if (type == PrimitiveType::AABB) {
					auto it =block.modelTextures.begin() + index * 6;
					block.modelTextures.erase(it, it + 6);
				}
				aabbArr.erase(aabbArr.begin() + index);
				createCustomModelEditor(block, std::min(aabbArr.size() - 1, index), type);
			}
			if (type != PrimitiveType::HITBOX) preview->updateCache();
		}));
		panel->add(std::make_shared<gui::Button>(L"Previous", glm::vec4(10.f), [this, &block, index, type](gui::GUI*) {
			if (index != 0) {
				createCustomModelEditor(block, index - 1, type);
			}
		}));
		panel->add(std::make_shared<gui::Button>(L"Next", glm::vec4(10.f), [this, &block, index, type, &aabbArr](gui::GUI*) {
			size_t place = (type == PrimitiveType::TETRAGON ? index * 4 + 4 : index + 1);
			size_t size = (type == PrimitiveType::TETRAGON ?block.modelExtraPoints.size() : aabbArr.size());
			if (place < size) {
				createCustomModelEditor(block, index + 1, type);
			}
		}));

		auto updateTetragon = [](glm::vec3* p) {
			glm::vec3 p1 = p[0];
			glm::vec3 xw = p[1] - p1;
			glm::vec3 yh = p[3] - p1;
			p[2] = p1 + xw + yh;
		};

		static unsigned int inputMode = 0;
		auto createInputModeButton = [](const std::function<void(unsigned int)>& callback) {
			std::wstring inputModes[] = { L"Slider", L"InputBox" };
			auto button = std::make_shared<gui::Button>(L"Input mode: " + inputModes[inputMode], glm::vec4(10.f), gui::onaction());
			button->listenAction([button, &m = inputMode, inputModes, callback](gui::GUI*) {
				if (++m >= 2) m = 0;
				button->setText(L"Input mode: " + inputModes[m]);
				callback(m);
				});
			return button;
		};

		auto inputPanel = std::make_shared<gui::Panel>(glm::vec2(panel->getSize().x));
		inputPanel->setColor(glm::vec4(0.f));
		panel->add(inputPanel);

		std::function<void(unsigned int)> createInput;
		std::string* textures = nullptr;
		if (type == PrimitiveType::TETRAGON) {
			preview->setCurrentTetragon(&block.modelExtraPoints[index * 4]);

			createInput = [this, &block, index, inputPanel, panel, updateTetragon](unsigned int type) {
				clearRemoveList(inputPanel);
				for (size_t i = 0; i < 4; i++) {
					if (i == 2) continue;
					glm::vec3* vec = &block.modelExtraPoints[index * 4];
					inputPanel->add(removeList.emplace_back(createVectorPanel(vec[i], 0.f, 1.f, panel->getSize().x, type,
						[this, updateTetragon, vec]() {
							updateTetragon(vec);
							preview->setCurrentTetragon(vec);
							preview->updateMesh();
						})));
				}
			};
			textures =block.modelTextures.data() +block.modelBoxes.size() * 6 + index;
		}
		else {
			AABB& aabb = aabbArr[index];
			preview->setCurrentAABB(aabb, type);

			createInput = [this, &aabb, inputPanel, panel, type](unsigned int inputType) {
				clearRemoveList(inputPanel);
				inputPanel->add(removeList.emplace_back(createVectorPanel(aabb.a, 0.f, 1.f, panel->getSize().x, inputType,
					[this, &aabb, type]() { preview->setCurrentAABB(aabb, type); preview->updateMesh(); })));
				inputPanel->add(removeList.emplace_back(createVectorPanel(aabb.b, 0.f, 1.f, panel->getSize().x, inputType,
					[this, &aabb, type]() { preview->setCurrentAABB(aabb, type); preview->updateMesh(); })));
			};
			textures =block.modelTextures.data() + index * 6;
		}
		createInput(inputMode);
		panel->add(createInputModeButton(createInput));
		if (type != PrimitiveType::HITBOX) {
			auto texturePanel = std::make_shared<gui::Panel>(glm::vec2(panel->getSize().x, 35.f));
			createTexturesPanel(texturePanel, 35.f, textures, (type == PrimitiveType::AABB ? BlockModel::aabb : BlockModel::xsprite));
			panel->add(texturePanel);
		}
		return panel;
	}, 3);
}