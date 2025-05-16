#pragma once

#include <functional>
#include <glm/fwd.hpp>
#include <string>

#include "frontend/workshop/WorkshopUtils.hpp"

namespace gui {
	class TextBox;
	class Container;
	class FullCheckBox;
}

namespace workshop {
	extern gui::TextBox& createTextBox(gui::Container& container, std::string& string, const std::wstring& placeholder = L"Type here");
	extern gui::FullCheckBox& createFullCheckBox(gui::Container& container, const std::wstring& string, bool& isChecked, const std::wstring& tooltip = L"");
	template<glm::length_t L, typename T>
	extern gui::Container& createVectorPanel(vec_t<L, T>& vec, vec_t<L, T> min, vec_t<L, T> max, float width, InputMode inputMode, const std::function<void()>& callback);
	extern void createEmissionPanel(gui::Container& container, uint8_t* emission);
	template<typename T>
	extern gui::TextBox& createNumTextBox(T& value, const std::wstring& placeholder, size_t floatPrecision = 3, T min = std::numeric_limits<T>::lowest(),
		T max = std::numeric_limits<T>::max() - 1, const std::function<void(T)>& callback = [](T) {});
}