#pragma once

#include <unordered_map>

#include "graphics/ui/elements/Panel.hpp"

namespace gui {
	class Button;

	class TabsContainer : public Panel {
	public:
		TabsContainer(glm::vec2 size);
		~TabsContainer() {};

		virtual void setSize(glm::vec2 size) override;

		gui::Button* addTab(const std::string& id, const std::wstring& name, gui::UINode* node);
		void switchTo(const std::string& id);

	private:
		gui::Container* tabsContainer = nullptr;
		std::unordered_map<std::string, std::shared_ptr<gui::UINode>> tabs;
	};
}