#include "TabsContainer.hpp"

#include "graphics/ui/elements/Button.hpp"
#include "frontend/workshop/WorkshopUtils.hpp"

using namespace workshop;

gui::TabsContainer::TabsContainer(glm::vec2 size) : Panel(size) {
	tabsContainer = new gui::Container(glm::vec2(size.x, 0.f));
	tabsContainer->setColor(glm::vec4(0.f));
	*this << tabsContainer;
	setColor(glm::vec4(0.f));
	setScrollable(false);
	setSizeFunc([this]() {
		return parent == nullptr ? this->size : parent->getSize();;
	});
}

void gui::TabsContainer::setSize(glm::vec2 size) {
	Panel::setSize(size);
	placeNodesHorizontally(*tabsContainer);
}

gui::Button* gui::TabsContainer::addTab(const std::string& id, const std::wstring& name, gui::UINode* node) {
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
	if (nodes.size() == 1) {
		switchTo(id);
	}
	return button;
}

void gui::TabsContainer::switchTo(const std::string& id) {
	const auto& found = tabs.find(id);
	if (found == tabs.end()) return;

	for (auto& node : tabsContainer->getNodes()) {
		deselect(*node);
	}
	setSelected(*tabsContainer->getNodes()[std::distance(tabs.begin(), found)]);
	if (nodes.size() > 1)
		remove(nodes[1]);
	add(found->second);
	found->second->act(1.f);
}