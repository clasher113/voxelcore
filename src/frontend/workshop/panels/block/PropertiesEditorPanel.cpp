#include "frontend/workshop/WorkshopScreen.hpp"

#include "graphics/ui/elements/Panel.hpp"
#include "graphics/ui/elements/Image.hpp"
#include "graphics/ui/elements/TextBox.hpp"
#include "graphics/ui/elements/Button.hpp"
#include "frontend/workshop/gui_elements/BasicElements.hpp"
#include "util/stringutil.hpp"

using namespace workshop;

template<typename T>
static void add(dv::value& dst, const std::string& key, T src) {
	if (dst.isObject()) {
		dst[key] = src;
	}
	else if (dst.isList()) {
		std::shared_ptr<dv::objects::List>& list = dst.getRef<std::shared_ptr<dv::objects::List>>(dst);
		if (key.empty()) list->emplace_back(src);
		else list->emplace(list->begin() + std::stoull(key), src);
	}
}

void WorkShopScreen::createPropertyEditor(gui::Panel& panel, dv::value& blockProps, const dv::value& definedProps, std::vector<size_t> path) {
	panel.clear();

	std::wstring pathStr = L"root";
	auto getCurrentObject = [&blockProps, path, &pathStr]() -> dv::value& {
		dv::value* result = &blockProps;
		for (const size_t index : path) {
			if (result->isObject()) {
				auto begin = const_cast<dv::objects::Object&>(result->asObject()).begin();
				std::advance(begin, index);
				result = &begin->second;
				pathStr.append(L"[\"" + util::str2wstr_utf8(begin->first) + L"\"]/");
			}
			else if (result->isList()) {
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
		if (isRoot && currentObject.isObject()) {
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
		else if (value.isBoolean()) {
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
			propPanel << new gui::Button(L"Click to explore", glm::vec4(8.f), [=, p = path, &panel, &definedProps, &blockProps](gui::GUI*) mutable {
				size_t index = currentObject.isObject() ? std::distance(currentObject.asObject().begin(), currentObject.asObject().find(key)) :
					std::stoull(key);
				p.emplace_back(index);
				createPropertyEditor(panel, blockProps, definedProps, p);
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

	if (!isRoot) {
		gui::Panel& navigationPanel = *new gui::Panel(glm::vec2(0, 38));
		navigationPanel.setColor(glm::vec4(0.5f));
		navigationPanel.setOrientation(gui::Orientation::horizontal);
		navigationPanel << new gui::Button(L"<- Back", glm::vec4(10.f), [=, p = path, &panel, &blockProps, &definedProps](gui::GUI*) mutable {
			p.pop_back();
			createPropertyEditor(panel, blockProps, definedProps, p);
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
		}
		else if (currentObject.isList()) {
			labelPanel << new gui::Label("Index");
		}
		labelPanel << new gui::Label("Value");
		labelPanel.setSizeFunc(getSizeFunc(labelPanel, objectPanelSizes));

		panel << labelPanel;
	}
	if (currentObject.isObject()) {
		for (auto& [key, value] : currentObject.asObject()) {
			if (isRoot && contains(DEFAULT_BLOCK_PROPERTIES, key)) continue;
			panel << createPropObject(key, const_cast<dv::value&>(value));
		}
	}
	else if (currentObject.isList()) {
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
			else if (isRoot && contains(DEFAULT_BLOCK_PROPERTIES, input)) tooltip = L"Default property name cannot be used";
			else if (currentObject.has(input)) tooltip = L"Property already exists";
		}
		else if (currentObject.isList()) {
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