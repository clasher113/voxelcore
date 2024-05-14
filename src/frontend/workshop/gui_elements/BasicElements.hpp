#ifndef FRONTEND_MENU_WORKSHOP_GUI_BASIC_ELEMENTS_HPP
#define FRONTEND_MENU_WORKSHOP_GUI_BASIC_ELEMENTS_HPP

#include <functional>
#include <glm/fwd.hpp>
#include <memory>
#include <string>

namespace gui {
	class TextBox;
	class Container;
	class FullCheckBox;
	class UINode;
	class Panel;
}

namespace workshop {
	extern std::shared_ptr<gui::TextBox> createTextBox(std::shared_ptr<gui::Container> container, std::string& string,
		const std::wstring& placeholder = L"Type here");
	extern std::shared_ptr<gui::FullCheckBox> createFullCheckBox(std::shared_ptr<gui::Container> container, const std::wstring& string, bool& isChecked);
	extern std::shared_ptr<gui::UINode> createVectorPanel(glm::vec3& vec, float min, float max, float width, unsigned int inputType, const std::function<void()>& callback);
	extern void createEmissionPanel(std::shared_ptr<gui::Container> container, uint8_t* emission);
	template<typename T>
	extern std::shared_ptr<gui::TextBox> createNumTextBox(T& value, const std::wstring& placeholder, T min, T max,
		const std::function<void(T)>& callback = [](T) {});
}

#endif // !FRONTEND_MENU_WORKSHOP_GUI_BASIC_ELEMENTS_HPP
