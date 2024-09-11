#include "BasicElements.hpp"

#include "../../../graphics/core/Atlas.hpp"
#include "../IncludeCommons.hpp"
#include "../WorkshopUtils.hpp"

gui::TextBox& workshop::createTextBox(gui::Container& container, std::string& string, const std::wstring& placeholder) {
	gui::TextBox& textBox = *new gui::TextBox(placeholder);
	textBox.setText(util::str2wstr_utf8(string));
	textBox.setTextConsumer([&string, &textBox](std::wstring text) {
		string = util::wstr2str_utf8(textBox.getInput());
	});
	textBox.setTextSupplier([&string]() {
		return util::str2wstr_utf8(string);
	});
	container += textBox;
	return textBox;
}

gui::FullCheckBox& workshop::createFullCheckBox(gui::Container& container, const std::wstring& string, bool& isChecked, const std::wstring& tooltip) {
	gui::FullCheckBox& checkbox = *new gui::FullCheckBox(string, glm::vec2(200, 24));
	checkbox.setSupplier([&isChecked]() {
		return isChecked;
	});
	checkbox.setConsumer([&isChecked](bool checked) {
		isChecked = checked;
	});
	container += checkbox;
	if (!tooltip.empty()) checkbox.setTooltip(tooltip);
	return checkbox;
}

template<glm::length_t L, typename T>
gui::Container& workshop::createVectorPanel(vec_t<L, T>& vec, vec_t<L, T> min, vec_t<L, T> max, float width, unsigned int inputType, const std::function<void()>& callback) {
	const std::wstring coords[] = { L"X", L"Y", L"Z" };
	if (inputType == 0) {
		gui::Panel& panel = *new gui::Panel(glm::vec2(width));
		panel.setColor(glm::vec4(0.f));
		auto coordsString = [coords](const vec_t<L, T>& vec) {
			std::wstring result;
			for (typename vec_t<L, T>::length_type i = 0; i < vec_t<L, T>::length(); i++) {
				result.append(coords[i] + L":" + util::to_wstring(vec[i], 2) + L" ");
			}
			return result;
		};
		gui::Label& label = *new gui::Label(coordsString(vec));
		panel += label;

		for (typename vec_t<L, T>::length_type i = 0; i < vec_t<L, T>::length(); i++) {
			gui::TrackBar& slider = *new gui::TrackBar(min[i], max[i], vec[i], 0.01f, 5);
			slider.setConsumer([&vec, i, coordsString, callback, &label](double value) {
				vec[i] = static_cast<T>(value);
				label.setText(coordsString(vec));
				callback();
			});
			panel += slider;
		}
		return panel;
	}
	gui::Container& container = *new gui::Container(glm::vec2(width, 0));

	for (typename vec_t<L, T>::length_type i = 0; i < vec_t<L, T>::length(); i++) {
		container += createNumTextBox(vec[i], coords[i], min[i], max[i], std::function<void(T)>([callback](T num) { callback(); }));
	}
	placeNodesHorizontally(container);

	return container;
}

template gui::Container& workshop::createVectorPanel(vec_t<3, float>& vec, vec_t<3, float> min, vec_t<3, float> max, float width, unsigned int inputType,
	const std::function<void()>& callback);
template gui::Container& workshop::createVectorPanel(vec_t<3, glm::i8>& vec, vec_t<3, glm::i8> min, vec_t<3, glm::i8> max, float width, unsigned int inputType,
	const std::function<void()>& callback);

void workshop::createEmissionPanel(gui::Container& container, uint8_t* emission) {
	container += new gui::Label("Emission (0 - 15)");
	const wchar_t* const colors[] = { L"Red", L"Green", L"Blue" };
	gui::Container& resultContainer = *new gui::Container(glm::vec2(container.getSize().x, 20));
	resultContainer.setColor(glm::vec4(emission[0] / 15.f, emission[1] / 15.f, emission[2] / 15.f, 1.f));
	gui::Container& vectorPanel = createVectorPanel(*reinterpret_cast<glm::u8vec3*>(emission), glm::u8vec3(0), glm::u8vec3(15),
		container.getSize().x, 1, [&resultContainer, emission]() {
		resultContainer.setColor(glm::vec4(emission[0] / 15.f, emission[1] / 15.f, emission[2] / 15.f, 1.f));
	});
	size_t index = 0;
	for (auto& elem : vectorPanel.getNodes()) {
		static_cast<gui::TextBox*>(elem.get())->setPlaceholder(colors[index++]);
	}
	container += vectorPanel;
	container += resultContainer;
}

template gui::TextBox& workshop::createNumTextBox(unsigned int&, const std::wstring&, unsigned int, unsigned int,
const std::function<void(unsigned int)>&);
template gui::TextBox& workshop::createNumTextBox(unsigned char&, const std::wstring&, unsigned char, unsigned char,
	const std::function<void(unsigned char)>&);
template gui::TextBox& workshop::createNumTextBox(float&, const std::wstring&, float, float, const std::function<void(float)>&);

template<typename T>
gui::TextBox& workshop::createNumTextBox(T& value, const std::wstring& placeholder, T min, T max, const std::function<void(T)>& callback) {
	gui::TextBox& textBox = *new gui::TextBox(placeholder);
	textBox.setText(std::to_wstring(value));
	textBox.setTextConsumer([&value, &textBox, min, max, callback](std::wstring text) {
		const std::wstring& input = textBox.getInput();
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
	textBox.setTextSupplier([&value, min]() {
		if (value != min) return util::to_wstring(value, (std::is_floating_point<T>::value ? 3 : 0));
		return std::wstring(L"");
	});
	return textBox;
}