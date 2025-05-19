#include "BasicElements.hpp"

#include "graphics/ui/elements/TextBox.hpp"
#include "graphics/ui/elements/CheckBox.hpp"
#include "graphics/ui/elements/TrackBar.hpp"
#include "util/stringutil.hpp"

gui::TextBox& workshop::createTextBox(gui::Container& container, std::string& string, const std::wstring& placeholder) {
	gui::TextBox& textBox = *new gui::TextBox(placeholder);
	textBox.setText(util::str2wstr_utf8(string));
	//textBox.setTextConsumer([&string, &textBox](std::wstring text) {
	//	string = util::wstr2str_utf8(textBox.getInput());
	//});
	textBox.setTextSupplier([&string, &textBox]() {
		if (textBox.validate()) string = util::str2str_utf8(textBox.getInput());
		return util::str2wstr_utf8(string);
	});
	container << textBox;
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
	container << checkbox;
	if (!tooltip.empty()) checkbox.setTooltip(tooltip);
	return checkbox;
}

template<glm::length_t L, typename T>
gui::Container& workshop::createVectorPanel(vec_t<L, T>& vec, vec_t<L, T> min, vec_t<L, T> max, float width, InputMode inputMode, const std::function<void()>& callback) {
	const std::wstring coords[] = { L"X", L"Y", L"Z" };
	if (inputMode == InputMode::SLIDER) {
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
		panel << label;

		for (typename vec_t<L, T>::length_type i = 0; i < vec_t<L, T>::length(); i++) {
			gui::TrackBar& slider = *new gui::TrackBar(min[i], max[i], vec[i], 0.01f, 5);
			slider.setConsumer([&vec, i, coordsString, callback, &label](double value) {
				vec[i] = static_cast<T>(value);
				label.setText(coordsString(vec));
				if (callback) callback();
			});
			panel << slider;
		}
		return panel;
	}
	gui::Container& container = *new gui::Container(glm::vec2(width, 0));

	for (typename vec_t<L, T>::length_type i = 0; i < vec_t<L, T>::length(); i++) {
		container << createNumTextBox(vec[i], coords[i] + L" (" + util::str2wstr_utf8(util::to_string(min[i])) + L")", 3, min[i], max[i], 
			std::function<void(T)>([callback](T num) { if (callback) callback(); }));
	}
	placeNodesHorizontally(container);

	return container;
}

template gui::Container& workshop::createVectorPanel(vec_t<3, float>& vec, vec_t<3, float> min, vec_t<3, float> max, float width, InputMode inputMode,
	const std::function<void()>& callback);
template gui::Container& workshop::createVectorPanel(vec_t<3, glm::i8>& vec, vec_t<3, glm::i8> min, vec_t<3, glm::i8> max, float width, InputMode inputMode,
	const std::function<void()>& callback);

void workshop::createEmissionPanel(gui::Container& container, uint8_t* emission) {
	container << new gui::Label("Emission (0 - 15)");
	const wchar_t* const colors[] = { L"Red (0)", L"Green (0)", L"Blue (0)" };
	gui::Container& resultContainer = *new gui::Container(glm::vec2(container.getSize().x, 20));
	resultContainer.setColor(glm::vec4(emission[0] / 15.f, emission[1] / 15.f, emission[2] / 15.f, 1.f));
	gui::Container& vectorPanel = createVectorPanel(*reinterpret_cast<glm::u8vec3*>(emission), glm::u8vec3(0), glm::u8vec3(15),
		container.getSize().x, InputMode::TEXTBOX, [&resultContainer, emission]() {
		resultContainer.setColor(glm::vec4(emission[0] / 15.f, emission[1] / 15.f, emission[2] / 15.f, 1.f));
	});
	size_t index = 0;
	for (auto& elem : vectorPanel.getNodes()) {
		static_cast<gui::TextBox*>(elem.get())->setPlaceholder(colors[index++]);
	}
	container << vectorPanel;
	container << resultContainer;
}

#define NUM_TEXTBOX(TYPE) template gui::TextBox& workshop::createNumTextBox(TYPE&, const std::wstring&, size_t, TYPE, TYPE, const std::function<void(TYPE)>&);
NUM_TEXTBOX(unsigned int)
NUM_TEXTBOX(unsigned char)
NUM_TEXTBOX(float)
NUM_TEXTBOX(double)
NUM_TEXTBOX(int64_t)
NUM_TEXTBOX(int)
#undef NUM_TEXTBOX

template<typename T>
gui::TextBox& workshop::createNumTextBox(T& value, const std::wstring& placeholder, size_t floatPrecision, T min, T max,
	const std::function<void(T)>& callback)
{
	gui::TextBox& textBox = *new gui::TextBox(placeholder);
	textBox.setTooltipDelay(0.f);
	textBox.setText(std::to_wstring(value));
	textBox.setTextValidator([&textBox, min, max](const std::wstring&) {
		std::wstring tooltip;
		const std::wstring& input = textBox.getInput();
		if (!input.empty()) {
			try {
				double result = min;
				if (std::is_floating_point<T>::value)
					result = stod(input);
				else result = stoll(input);
				if (result < min) tooltip = L"Number too low. Required minimum - " + std::to_wstring(min);
				else if (result > max) tooltip = L"Number too big. Possible maximum - " + std::to_wstring(max);
			}
			catch (const std::exception& e) {
				textBox.setTooltip(L"Number is invalid: " + util::str2wstr_utf8(e.what()));
				return false;
			}
		}
		textBox.setTooltip(tooltip);
		return tooltip.empty();
	});
	textBox.setTextConsumer([&value, &textBox, min, max, callback](const std::wstring&) {
		const std::wstring& input = textBox.getInput();
		if (input.empty()) {
			value = min;
			if (callback) callback(value);
			return;
		}
		if (textBox.validate()) {
			if (std::is_floating_point<T>::value)
				value = static_cast<T>(stod(input));
			else value = static_cast<T>(stoll(input));
			if (callback) callback(value);
		}
	});
	textBox.setTextSupplier([&value, &textBox, min, floatPrecision]() {
		if (textBox.validate() && value != min) return util::to_wstring(value, (std::is_floating_point<T>::value ? floatPrecision : 0));
		return std::wstring(L"");
	});
	return textBox;
}