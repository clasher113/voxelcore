#include "BasicElements.hpp"

#include "../../../graphics/core/Atlas.hpp"
#include "../IncludeCommons.hpp"
#include "../WorkshopUtils.hpp"

std::shared_ptr<gui::TextBox> workshop::createTextBox(std::shared_ptr<gui::Container> container, std::string& string, const std::wstring& placeholder) {
	auto textBox = std::make_shared<gui::TextBox>(placeholder);
	textBox->setText(util::str2wstr_utf8(string));
	textBox->setTextConsumer([&string, textBox](std::wstring text) {
		string = util::wstr2str_utf8(textBox->getInput());
	});
	textBox->setTextSupplier([&string]() {
		return util::str2wstr_utf8(string);
	});
	container->add(textBox);
	return textBox;
}

std::shared_ptr<gui::FullCheckBox> workshop::createFullCheckBox(std::shared_ptr<gui::Container> container, const std::wstring& string, bool& isChecked) {
	auto checkbox = std::make_shared<gui::FullCheckBox>(string, glm::vec2(200, 24));
	checkbox->setSupplier([&isChecked]() {
		return isChecked;
	});
	checkbox->setConsumer([&isChecked](bool checked) {
		isChecked = checked;
	});
	container->add(checkbox);
	return checkbox;
}

std::shared_ptr<gui::UINode> workshop::createVectorPanel(glm::vec3& vec, float min, float max, float width, unsigned int inputType,
	const std::function<void()>& callback)
{
	const std::wstring coords[] = { L"X", L"Y", L"Z" };
	if (inputType == 0) {
		auto panel = std::make_shared<gui::Panel>(glm::vec2(width));
		panel->setColor(glm::vec4(0.f));
		auto coordsString = [coords](const glm::vec3& vec) {
			std::wstring result;
			for (glm::vec3::length_type i = 0; i < 3; i++) {
				result.append(coords[i] + L":" + util::to_wstring(vec[i], 2) + L" ");
			}
			return result;
		};
		auto label = std::make_shared<gui::Label>(coordsString(vec));
		panel->add(label);

		for (glm::vec3::length_type i = 0; i < 3; i++) {
			auto slider = std::make_shared<gui::TrackBar>(min, max, vec[i], 0.01f, 5);
			slider->setConsumer([&vec, i, coordsString, callback, label](double value) {
				vec[i] = static_cast<float>(value);
				label->setText(coordsString(vec));
				callback();
			});
			panel->add(slider);
		}
		return panel;
	}
	auto container = std::make_shared<gui::Container>(glm::vec2(0));

	for (glm::vec3::length_type i = 0; i < 3; i++) {
		container->add(createNumTextBox(vec[i], coords[i], min, max, std::function<void(float)>([callback](float num) { callback(); })));
	}
	float size = width / 3;
	float height = 0.f;
	size_t i = 0;
	for (auto& elem : container->getNodes()) {
		elem->setSize(glm::vec2(size - elem->getMargin().x - 4, elem->getSize().y));
		elem->setPos(glm::vec2(size * i++, 0.f));
		height = elem->getSize().y;
	}
	container->setSize(glm::vec2(width, height));
	container->setScrollable(false);

	return container;
}

void workshop::createEmissionPanel(std::shared_ptr<gui::Container> container, uint8_t* emission) {
	container->add(std::make_shared<gui::Label>("Emission (0 - 15)"));
	const wchar_t* colors[] = { L"Red", L"Green", L"Blue" };
	for (size_t i = 0; i < 3; i++) {
		container->add(createNumTextBox<uint8_t>(emission[i], colors[i], 0, 15));
	}
}

template std::shared_ptr<gui::TextBox> workshop::createNumTextBox(unsigned int&, const std::wstring&, unsigned int, unsigned int,
	const std::function<void(unsigned int)>&);
template std::shared_ptr<gui::TextBox> workshop::createNumTextBox(unsigned char&, const std::wstring&, unsigned char, unsigned char,
	const std::function<void(unsigned char)>&);
template std::shared_ptr<gui::TextBox> workshop::createNumTextBox(float&, const std::wstring&, float, float, const std::function<void(float)>&);

template<typename T>
std::shared_ptr<gui::TextBox> workshop::createNumTextBox(T& value, const std::wstring& placeholder, T min, T max,
	const std::function<void(T)>& callback)
{
	auto textBox = std::make_shared<gui::TextBox>(placeholder);
	textBox->setText(std::to_wstring(value));
	textBox->setTextConsumer([&value, textBox, min, max, callback](std::wstring text) {
		const std::wstring& input = textBox->getInput();
		if (input.empty()) {
			value = min;
			callback(value);
			return;
		}
		try {
			T result = static_cast<T>(stof(input));
			if (result >= min && result <= max) {
				value = result;
				callback(value);
			}
		}
		catch (const std::exception&) {}
	});
	textBox->setTextSupplier([&value, min]() {
		if (value != min) return util::to_wstring(value, (std::is_floating_point<T>::value ? 3 : 0));
		return std::wstring(L"");
	});
	return textBox;
}