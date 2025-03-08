//#include "DropDownList.hpp"
//
//#include "graphics/ui/elements/Button.hpp"
//
//gui::DropDownList::DropDownList(const std::wstring& text, const glm::vec2& size) : Container(glm::vec2(100.f))
//{
//	auto button = std::make_shared<Button>(text, glm::vec4(10.f), onaction());
//	button->listenAction([this, button](gui::GUI*) {
//		isOpen = !isOpen;
//	});
//	add(button);
//
//	listenInterval(0.01f, [this, button]() {
//		float maxSize = button->getSize().y + panel->getSize().y;
//		float minSize = button->getSize().y;
//
//		if (isOpen &&  this->size.y < maxSize) {
//			this->size.y += 10.f;
//		}
//		if (!isOpen && this->size.y > minSize){
//			this->size.y -= 10.f;
//		}
//	});
//
//	panel = std::make_shared<Panel>(glm::vec2(size.x, 0.f));
//	panel->setPos(glm::vec2(0.f, button->getSize().y));
//	add(panel);
//	for (size_t i = 0; i < 3; i++) {
//		addSelectable(std::to_wstring(i));
//	}
//	setSize(glm::vec2(size.x, button->getSize().y));
//	setScrollable(false);
//}
//
//void gui::DropDownList::addSelectable(const std::wstring& text) {
//	auto button = std::make_shared<Button>(text, glm::vec4(10.f), onaction());
//	panel->add(button);
//}
