#include "frontend/workshop/WorkshopScreen.hpp"

#include "graphics/ui/elements/Panel.hpp"
#include "graphics/ui/elements/Label.hpp"
#include "graphics/ui/elements/TextBox.hpp"
#include "graphics/ui/elements/Button.hpp"
#include "graphics/ui/elements/Image.hpp"
#include "frontend/workshop/gui_elements/BasicElements.hpp"
#include "data/StructLayout.hpp"
#include "util/stringutil.hpp"
#include "voxels/Block.hpp"

using namespace workshop;

void WorkShopScreen::createBlockFieldsEditor(gui::Panel& panel, std::unique_ptr<data::StructLayout>& fields) {
	panel.clear();

	if (fields == nullptr) {
		fields = std::make_unique<data::StructLayout>();
	}
	//													type			name		elements	convert		delete image
	const std::vector<glm::vec2> fieldPanelSizes = { { 50, 80 }, { 150, 5000 }, { 150, 150 }, { 150, 150 }, { 32, 32 } };
	const std::vector<glm::vec2> createFieldPanelSizes = { { 50, 80 }, { 150, 5000 }, { 150, 150 } };

	std::vector<data::Field>& fieldsArr = fields->getFields();

	const size_t size = fields->size();
	const size_t maxSize = MAX_USER_BLOCK_FIELDS_SIZE;

	if (fieldsArr.size()) {
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
			if (input.empty()) {
				tooltip = L"Input must not be empty";
			}
			else if (std::find_if(fieldsArr.begin(), fieldsArr.end(), [input, &field](const data::Field& a) {
				return a.name == input && field != a;
			}) != fieldsArr.end()) {
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
		fieldPanel << createNumTextBox<int>(field.elements, L">1 = array", 0, 1, max, [=, &panel, &fields, &fieldsArr](int) {
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