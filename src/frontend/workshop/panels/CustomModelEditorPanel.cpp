#include "../WorkshopScreen.hpp"

#include "../IncludeCommons.hpp"
#include "../WorkshopPreview.hpp"
#include "core_defs.hpp"
#include "window/Window.hpp"
#include "coders/json.hpp"
#include "data/StructLayout.hpp"

#include <algorithm>

using namespace workshop;

namespace gui {
	class TabsContainer : public Panel {
	public:
		TabsContainer(glm::vec2 size) : Panel(size){
			tabsContainer = new gui::Container(glm::vec2(size.x, 0.f));
			tabsContainer->setColor(glm::vec4(0.f));
			*this << tabsContainer;
			setColor(glm::vec4(0.f));
			setScrollable(false);
			setSizeFunc([this]() {
				return parent == nullptr ? this->size : parent->getSize();;
			});
		}
		~TabsContainer() {};

		virtual void setSize(glm::vec2 size) override {
			Panel::setSize(size);
			placeNodesHorizontally(*tabsContainer);
		}

		gui::Button* addTab(const std::string& id, const std::wstring& name, gui::UINode* node){
			gui::Button* button = new gui::Button(name, glm::vec4(10.f), [this, id](gui::GUI*) {
				switchTo(id);
			});
			tabsContainer->setSize(glm::vec2(tabsContainer->getSize().x, button->getSize().y));
			*tabsContainer << button;

			std::shared_ptr<gui::UINode> nodePtr(node);

			if (tabs.find(id) != tabs.end() && nodes[1] == tabs[id])
				remove(nodes[1]);

			tabs[id] = nodePtr;
			nodePtr->setSizeFunc([this]() {
				return glm::vec2(this->size.x, this->size.y - tabsContainer->getSize().y);
			});
			
			placeNodesHorizontally(*tabsContainer);
			setSelectable<gui::Button>(*tabsContainer);
			if (nodes.size() == 1){
				switchTo(id);
			}
			return button;
		}
		void switchTo(const std::string& id){
			auto& found = tabs.find(id);
			if (found == tabs.end()) return;

			for (auto& node : tabsContainer->getNodes()){
				deselect(*node);
			}
			setSelected(*tabsContainer->getNodes()[std::distance(tabs.begin(), found)]);
			if (nodes.size() > 1)
				remove(nodes[1]);
			add(found->second);
			found->second->act(1.f);
		}

	private:
		gui::Container* tabsContainer = nullptr;
		std::unordered_map<std::string, std::shared_ptr<gui::UINode>> tabs;
	};
}

static InputMode inputMode = InputMode::SLIDER;
static PrimitiveType modelPrimitive = PrimitiveType::AABB;
static std::vector<glm::vec3> points;
static std::vector<std::string> textures;

static size_t getPrimitivesCount(const Block& block, PrimitiveType type){
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

static void fetchModelData(const Block& block, PrimitiveType type){
	points.clear();
	textures.clear();

	if (type == PrimitiveType::AABB){
		if (!block.customModelRaw.has(AABB_STR)) return;
		const dv::value& aabbs = block.customModelRaw[AABB_STR];
		for (const dv::value& aabb : aabbs){
			points.emplace_back(exportAABB(aabb).a);
			points.emplace_back(exportAABB(aabb).b);
			if (aabb.size() == 7) {
				for (uint j = 6; j < 12; j++) {
					textures.emplace_back(aabb[6].asString());
				}
			} else if (aabb.size() == 12) {
				for (uint j = 6; j < 12; j++) {
					textures.emplace_back(aabb[j].asString());
				}
			}
		}

	} else if (type == PrimitiveType::TETRAGON){
		if (!block.customModelRaw.has(TETRAGON_STR)) return;
		const dv::value& tetragons = block.customModelRaw[TETRAGON_STR];
		for (const dv::value& tetragon : tetragons) {
			for (const glm::vec3& point : exportTetragon(tetragon)){
				points.emplace_back(point);
			}
			textures.emplace_back(tetragon[9].asString());
		}
	}
}

static void applyModelData(Block& block, PrimitiveType type){
	if (points.empty()) return;
	const std::string currentPrimitiveId = type == PrimitiveType::AABB ? AABB_STR : TETRAGON_STR;
	if (!block.customModelRaw.has(currentPrimitiveId))
		block.customModelRaw[currentPrimitiveId] = dv::list();
	dv::value& primitives = block.customModelRaw[currentPrimitiveId];
	while (!primitives.empty()){
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

void workshop::WorkShopScreen::createPrimitiveEditor(gui::Panel& panel, Block& block, size_t index, PrimitiveType type) {
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

template<typename T>
static void add(dv::value& dst, const std::string& key, T src){
	if (dst.isObject()){
		dst[key] = src;
	} else if (dst.isList()) {
		std::shared_ptr<dv::objects::List>& list = dst.getRef<std::shared_ptr<dv::objects::List>>(dst);
		if (key.empty()) list->emplace_back(src);
		else list->emplace(list->begin() + std::stoull(key), src);
	}
}

static std::function<glm::vec2()> getSizeFunc(gui::Panel& panel, const std::vector<glm::vec2>& sizes){
	return [&panel, sizes]() {
		auto& nodes = panel.getNodes();
		float totalWidth = panel.getSize().x - 2.f - (2.f * nodes.size());
		const size_t count = std::min(nodes.size(), sizes.size());
		std::vector<size_t> sizesCalculated;

		// todo optimize
		for (const glm::vec2& size : sizes) {
			sizesCalculated.emplace_back(size.x);
			totalWidth -= size.x;
		}
		while (totalWidth > 0) {
			for (size_t i = 0; i < count; i++) {
				if (sizesCalculated[i] < sizes[i].y) {
					sizesCalculated[i]++;
					totalWidth--;
				}
				if (totalWidth <= 0) break;
			}
		}
		for (size_t i = 0; i < count; i++) {
			nodes[i]->setSize(glm::vec2(sizesCalculated[i], panel.getSize().y - 4.f));
		}

		return panel.getSize();
	};
}

static void createPropertyEditor(gui::Panel& panel, dv::value& blockProps, const dv::value& definedProps, std::vector<size_t> path = {}) {
	panel.clear();

	std::wstring pathStr = L"root";
	auto getCurrentObject = [&blockProps, path, &pathStr]() -> dv::value& {
		dv::value* result = &blockProps;
		for (const size_t index : path) {
			if (result->isObject()){
				auto begin = const_cast<dv::objects::Object&>(result->asObject()).begin();
				std::advance(begin, index);
				result = &begin->second;
				pathStr.append(L"[\"" + util::str2wstr_utf8(begin->first) + L"\"]/");
			} else if (result->isList()){
				result = &const_cast<dv::value&>(result->asList()[index]);
				pathStr.append(L"list[" + std::to_wstring(index) + L"]/");
			}
		}
		return *result;
	};
	dv::value& currentObject = getCurrentObject();
	const bool isRoot = path.empty();
	//												status image	type			name		value		delete image
	const std::vector<glm::vec2> objectPanelSizes = { { 32, 32 }, { 80, 120 }, { 100, 300 }, { 100, 5000 }, { 32, 32 } };
	const std::vector<glm::vec2> createPropPanelSizes = { { 100, 150 }, { 100, 300 }, { 100, 5000 } };
	const std::vector<glm::vec2> navigationPanelSizes = { { 100, 150 }, { 200, 5000 } };

	auto createPropObject = [=, &currentObject, &definedProps, &panel, &blockProps](const std::string& key, dv::value& value) {
		gui::Panel& propPanel = *new gui::Panel(glm::vec2(0, 38));
		propPanel.setColor(glm::vec4(0.5f));
		propPanel.setOrientation(gui::Orientation::horizontal);
		gui::Image& image = *new gui::Image("blocks:transparent");
		if (isRoot && currentObject.isObject()){
			if (definedProps.has(key)) {
				image.setTexture("gui/circle");
				image.setColor(glm::vec4(0.f, 1.f, 0.f, 1.f));
			}
			else {
				image.setTexture("gui/warning");
				propPanel.setTooltip(L"Not defined in user-props.toml");
			}
		}
		propPanel << image;

		gui::TextBox* textBox = new gui::TextBox(L"");
		textBox->setText(util::str2wstr_utf8(dv::type_name(value.getType())));
		textBox->setEditable(false);
		propPanel << textBox;

		textBox = new gui::TextBox(L"");
		textBox->setText(util::str2wstr_utf8(key));
		textBox->setEditable(false);
		propPanel << textBox;

		if (value.isString())
			createTextBox(propPanel, const_cast<std::string&>(value.asString()));
		else if (value.isNumber())
			propPanel << createNumTextBox(dv::value::getRef<dv::number_t>(value), L"", 10);
		else if (value.isInteger())
			propPanel << createNumTextBox(dv::value::getRef<dv::integer_t>(value), L"");
		else if (value.isBoolean()){
			auto bool2str = [](bool b) {
				return b ? L"true" : L"false";
			};
			gui::Button& boolButton = *new gui::Button(bool2str(value.asBoolean()), glm::vec4(8.f), gui::onaction());
			boolButton.setTextAlign(gui::Align::left);
			boolButton.listenAction([=, &value, &boolButton](gui::GUI*) {
				value = !value.asBoolean();
				boolButton.setText(bool2str(value.asBoolean()));
			});
			propPanel << boolButton;
		}
		else { // object / list
			propPanel << new gui::Button(L"Click to explore", glm::vec4(8.f), [=, &panel, &definedProps, &blockProps](gui::GUI*) mutable {
				size_t index = currentObject.isObject() ? std::distance(currentObject.asObject().begin(), currentObject.asObject().find(key)) :
					std::stoull(key);
				path.emplace_back(index);
				createPropertyEditor(panel, blockProps, definedProps, path);
			});
		}

		gui::Button& deleteButton = *new gui::Button(std::make_shared<gui::Image>("gui/delete_icon"));
		deleteButton.listenAction([=, &panel, &blockProps, &definedProps, &currentObject](gui::GUI*) {
			if (currentObject.isObject()) currentObject.erase(key);
			else if (currentObject.isList()) currentObject.erase(std::stoull(key));
			createPropertyEditor(panel, blockProps, definedProps, path);
		});
		propPanel << deleteButton;

		propPanel.setSizeFunc(getSizeFunc(propPanel, objectPanelSizes));

		return std::ref(propPanel);
	};

	if (!isRoot){
		gui::Panel& navigationPanel = *new gui::Panel(glm::vec2(0, 38));
		navigationPanel.setColor(glm::vec4(0.5f));
		navigationPanel.setOrientation(gui::Orientation::horizontal);
		navigationPanel << new gui::Button(L"<- Back", glm::vec4(10.f), [=, &panel, &blockProps, &definedProps](gui::GUI*) mutable {
			path.pop_back();
			createPropertyEditor(panel, blockProps, definedProps, path);
		});
		gui::TextBox& textBox = *new gui::TextBox(L"");
		textBox.setText(L"Exploring: " + pathStr);
		textBox.setEditable(false);
		navigationPanel << textBox;

		navigationPanel.setSizeFunc(getSizeFunc(navigationPanel, navigationPanelSizes));

		panel << navigationPanel;
	}

	if (!currentObject.empty()) {
		gui::Panel& labelPanel = *new gui::Panel(glm::vec2(0, 30));
		labelPanel.setColor(glm::vec4(0.5f));
		labelPanel.setOrientation(gui::Orientation::horizontal);
		labelPanel << new gui::Image("blocks:transparent");
		labelPanel << new gui::Label("Type");
		if (currentObject.isObject()) {
			labelPanel << new gui::Label("Name");
		} else if (currentObject.isList()){
			labelPanel << new gui::Label("Index");
		}
		labelPanel << new gui::Label("Value");
		labelPanel.setSizeFunc(getSizeFunc(labelPanel, objectPanelSizes));

		panel << labelPanel;
	}
	if (currentObject.isObject()) {
		for (auto& [key, value] : currentObject.asObject()) {
			panel << createPropObject(key, const_cast<dv::value&>(value));
		}
	} else if (currentObject.isList()){
		dv::objects::List& list = const_cast<dv::objects::List&>(currentObject.asList());
		for (size_t i = 0; i < list.size(); i++) {
			panel << createPropObject(std::to_string(i), list[i]);
		}
	}

	gui::Panel& createPropPanel = *new gui::Panel(glm::vec2(0, 38));
	createPropPanel.setColor(glm::vec4(0.5f));
	createPropPanel.setOrientation(gui::Orientation::horizontal);
	static dv::value_type type = dv::value_type::number;
	gui::Button* typeButton = new gui::Button(L"Type: " + util::str2wstr_utf8(dv::type_name(type)), glm::vec4(10.f), gui::onaction());
	typeButton->listenAction([=](gui::GUI*) {
		incrementEnumClass(type, 1);
		if (type == dv::value_type::bytes) incrementEnumClass(type, 1);
		if (type > dv::value_type::string) type = dv::value_type::number;
		typeButton->setText(L"Type: " + util::str2wstr_utf8(dv::type_name(type)));
	});
	createPropPanel << typeButton;
	gui::TextBox* textBox = new gui::TextBox(L"");
	textBox->setHint(currentObject.isObject() ? L"Name" : L"Index (empty = last)");
	textBox->setTooltipDelay(0.f);
	textBox->setTextValidator([=, &currentObject](const std::wstring&) {
		const std::string input(util::wstr2str_utf8(textBox->getInput()));
		std::wstring tooltip;
		if (currentObject.isObject()) {
			if (input.empty()) tooltip = L"Input must not be empty";
			else if (currentObject.isObject()) {
				if (currentObject.has(input)) tooltip = L"Property already exists";
			}
			else if (isRoot) {
				for (const std::string& name : DEFAULT_BLOCK_PROPERTIES) {
					if (input == name) {
						tooltip = L"Default property name cannot be used";
						break;
					}
				}
			}
		} else if (currentObject.isList()){
			if (!input.empty()) {
				try {
					long long index = std::stoll(input);
					size_t maxSize = currentObject.size();
					if (index < 0) tooltip = L"Index too low (required minimum 0)";
					else if (index > maxSize) tooltip = L"Index too big (allowed maximum " + std::to_wstring(maxSize) + L" )";
				}
				catch (const std::exception& e) {
					tooltip = util::str2wstr_utf8(e.what());
				}
			}
		}
		textBox->setTooltip(tooltip);
		return tooltip.empty();
	});
	createPropPanel << textBox;
	createPropPanel << new gui::Button(L"Create new", glm::vec4(10.f), [=, &panel, &blockProps, &currentObject](gui::GUI*) {
		if (!textBox->validate()) return;
		const std::string name = util::wstr2str_utf8(textBox->getInput());
		bool isObject = currentObject.isObject();
		switch (type) {
			case dv::value_type::number:
				add(currentObject, name, 0.0);
				break;
			case dv::value_type::boolean:
				add(currentObject, name, false);
				break;
			case dv::value_type::integer:
				add(currentObject, name, 0ULL);
				break;
			case dv::value_type::object:
				add(currentObject, name, dv::object());
				break;
			case dv::value_type::list:
				add(currentObject, name, dv::list());
				break;
			case dv::value_type::string:
				add(currentObject, name, std::string());
				break;
		}
		createPropertyEditor(panel, blockProps, definedProps, path);
	});

	createPropPanel.setSizeFunc(getSizeFunc(createPropPanel, createPropPanelSizes));

	panel << createPropPanel;
}

void workshop::WorkShopScreen::createBlockFieldsEditor(gui::Panel& panel, std::unique_ptr<data::StructLayout>& fields) {
	panel.clear();

	if (fields == nullptr){
		fields = std::make_unique<data::StructLayout>();
	}
	//													type			name		elements	convert		delete image
	const std::vector<glm::vec2> fieldPanelSizes = { { 50, 80 }, { 150, 5000 }, { 150, 150 }, { 150, 150 }, {32, 32} };
	const std::vector<glm::vec2> createFieldPanelSizes = { { 50, 80 }, { 150, 5000 }, { 150, 150 } };

	std::vector<data::Field>& fieldsArr = fields->getFields();
	std::sort(fieldsArr.begin(), fieldsArr.end(), [](const data::Field& a, const data::Field& b) {
		return a.name < b.name;
	});
	const size_t size = fields->size();
	const size_t maxSize = MAX_USER_BLOCK_FIELDS_SIZE;

	if (fieldsArr.size()){
		panel << new gui::Label("Used " + std::to_string(size) + " byte(s) of " + std::to_string(maxSize) + " (" +
			std::to_string(static_cast<int>(static_cast<float>(size) / maxSize * 100)) + "%)");

		gui::Panel& labelPanel = *new gui::Panel(glm::vec2(0, 30));
		labelPanel.setColor(glm::vec4(0.5f));
		labelPanel.setOrientation(gui::Orientation::horizontal);
		labelPanel << new gui::Label("Type");
		labelPanel << new gui::Label("Name");
		labelPanel << new gui::Label("Elements count");
		labelPanel << new gui::Label("Convert strategy");
		labelPanel.setSizeFunc(getSizeFunc(labelPanel, fieldPanelSizes));
		panel << labelPanel;
	}

	for (size_t i = 0; i < fieldsArr.size(); i++) {
		data::Field& field = fieldsArr[i];
		gui::Panel& fieldPanel = *new gui::Panel(glm::vec2(0, 38));
		fieldPanel.setColor(glm::vec4(0.5f));
		fieldPanel.setOrientation(gui::Orientation::horizontal);

		gui::TextBox* textBox = new gui::TextBox(L"");
		textBox->setText(util::str2wstr_utf8(to_string(field.type)));
		textBox->setEditable(false);
		fieldPanel << textBox;

		textBox = new gui::TextBox(L"");
		textBox->setTooltipDelay(0.f);
		textBox->setText(util::str2wstr_utf8(field.name));
		textBox->setTextValidator([=, &fieldsArr, &fields](const std::wstring&) {
			std::string input(util::wstr2str_utf8(textBox->getInput()));
			std::wstring tooltip;
			if (input.empty()){	
				tooltip = L"Input must not be empty";
			}
			else if (std::find_if(fieldsArr.begin(), fieldsArr.end(), [input, &field](const data::Field& a) {
				return a.name == input && field != a;
			}) != fieldsArr.end()){
				tooltip = L"Name already in use";
			}
			textBox->setTooltip(tooltip);
			return tooltip.empty();
		});
		textBox->setTextConsumer([=, &panel, &fields, &field](std::wstring text) {
			field.name = util::wstr2str_utf8(textBox->getInput());
			createBlockFieldsEditor(panel, fields);
		});
		textBox->setTextSupplier([&field]() {
			return util::str2wstr_utf8(field.name);
		});
		fieldPanel << textBox;

		const size_t max = (maxSize - size + field.size) / data::sizeof_type(field.type);
		fieldPanel << createNumTextBox<int>(field.elements, L">1 = array", 0, 1, max, [=, &panel, &fields, &fieldsArr](int){
			fields.reset(new data::StructLayout(data::StructLayout::create(fieldsArr)));
			createBlockFieldsEditor(panel, fields);
		});

		gui::Button& button = *new gui::Button(util::str2wstr_utf8(to_string(field.convertStrategy)), glm::vec4(10.f), gui::onaction());
		button.listenAction([&button, &field](gui::GUI*) {
			incrementEnumClass(field.convertStrategy, 1);
			if (field.convertStrategy > data::FieldConvertStrategy::CLAMP) field.convertStrategy = data::FieldConvertStrategy::RESET;
			button.setText(util::str2wstr_utf8(to_string(field.convertStrategy)));
		});
		fieldPanel << button;

		gui::Button& deleteButton = *new gui::Button(std::make_shared<gui::Image>("gui/delete_icon"));
		deleteButton.listenAction([=, &panel, &fields, &fieldsArr](gui::GUI*) {
			fieldsArr.erase(fieldsArr.begin() + i);
			fields.reset(new data::StructLayout(data::StructLayout::create(fieldsArr)));
			createBlockFieldsEditor(panel, fields);
		});
		fieldPanel << deleteButton;

		fieldPanel.setSizeFunc(getSizeFunc(fieldPanel, fieldPanelSizes));

		panel << fieldPanel;
	}

	gui::Panel& addFieldPanel = *new gui::Panel(glm::vec2(0.f, 38.f));
	addFieldPanel.setColor(glm::vec4(0.5f));
	addFieldPanel.setOrientation(gui::Orientation::horizontal);

	static data::FieldType type = data::FieldType::I8;
	gui::Button& typeButton = *new gui::Button(util::str2wstr_utf8(to_string(type)), glm::vec4(8.f), gui::onaction());
	typeButton.setTextAlign(gui::Align::left);
	typeButton.listenAction([=, &typeButton](gui::GUI*) {
		incrementEnumClass(type, 1);
		if (type > data::FieldType::CHAR) type = data::FieldType::I8;
		typeButton.setText(util::str2wstr_utf8(to_string(type)));
	});
	addFieldPanel << typeButton;

	gui::TextBox* textBox = new gui::TextBox(L"");
	textBox->setHint(L"Enter name");
	textBox->setTooltipDelay(0.f);
	textBox->setTextValidator([=, &fieldsArr, &fields](const std::wstring&) {
		std::string input(util::wstr2str_utf8(textBox->getInput()));
		std::wstring tooltip;
		if (input.empty()) {
			tooltip = L"Input must not be empty";
		}
		else if (std::find_if(fieldsArr.begin(), fieldsArr.end(), [input](const data::Field& a) {
			return a.name == input;
		}) != fieldsArr.end()) {
			tooltip = L"Name already in use";
		}
		textBox->setTooltip(tooltip);
		return tooltip.empty();
	});
	addFieldPanel << textBox;

	addFieldPanel << new gui::Button(L"Create new", glm::vec4(10.f), [=, &fieldsArr, &panel, &fields](gui::GUI*) {
		if (!textBox->validate()) return;
		fieldsArr.emplace_back(type, util::wstr2str_utf8(textBox->getInput()), 1);
		fields.reset(new data::StructLayout(data::StructLayout::create(fieldsArr)));
		createBlockFieldsEditor(panel, fields);
	});

	addFieldPanel.setSizeFunc(getSizeFunc(addFieldPanel, createFieldPanelSizes));

	panel << addFieldPanel;
}

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

		if (block.model == BlockModel::custom){
			panel = new gui::Panel(glm::vec2());
			panel->setColor(glm::vec4(0.f));
			createPrimitiveEditor(*panel, block, index, PrimitiveType::AABB);
			gui::Button* modelButton = tabsContainer.addTab("model", L"Model", panel);
			modelButton->listenAction([=](gui::GUI*) {
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
		
		return std::ref(mainPanel);
	}, 3);
}