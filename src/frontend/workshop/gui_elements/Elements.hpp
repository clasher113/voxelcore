#ifndef FRONTEND_MENU_WORKSHOP_GUI_ELEMENTS_HPP
#define FRONTEND_MENU_WORKSHOP_GUI_ELEMENTS_HPP

#include <functional>
#include <glm/fwd.hpp>
#include <memory>
#include <string>

class Atlas;
namespace gui {
	class TextBox;
	class Panel;
	class FullCheckBox;
	class RichButton;
	class UINode;
}

namespace workshop {
	extern std::shared_ptr<gui::TextBox> createTextBox(std::shared_ptr<gui::Panel> panel, std::string& string,
		const std::wstring& placeholder = L"Type here");
	extern std::shared_ptr<gui::FullCheckBox> createFullCheckBox(std::shared_ptr<gui::Panel> panel, const std::wstring& string, bool& isChecked);
	extern std::shared_ptr<gui::RichButton> createTextureButton(const std::string& texture, Atlas* atlas, glm::vec2 size, const wchar_t* side = nullptr);
	extern std::shared_ptr<gui::UINode> createVectorPanel(glm::vec3& vec, float min, float max, float width, unsigned int inputType, const std::function<void()>& callback);
	extern void createEmissionPanel(std::shared_ptr<gui::Panel> panel, uint8_t* emission);
	template<typename T>
	extern std::shared_ptr<gui::TextBox> createNumTextBox(T& value, const std::wstring& placeholder, T min, T max,
		const std::function<void(T)>& callback = [](T) {});
}

#endif // !FRONTEND_MENU_WORKSHOP_GUI_ELEMENTS_HPP
