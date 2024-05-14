#include "menu_workshop.hpp"

#include "../../engine.hpp"
#include "../../graphics/ui/elements/Button.hpp"
#include "../../graphics/ui/elements/commons.hpp"
#include "../../graphics/ui/elements/Label.hpp"
#include "../../graphics/ui/elements/Menu.hpp"
#include "../../graphics/ui/elements/Panel.hpp"
#include "../../graphics/ui/elements/Textbox.hpp"
#include "../../graphics/ui/GUI.hpp"
#include "../../graphics/ui/gui_util.hpp"
#include "../../util/stringutil.hpp"
#include "WorkshopScreen.hpp"
#include "WorkshopSerializer.hpp"

void workshop::create_workshop_button(Engine* engine) {
	auto menu = engine->getGUI()->getMenu();
	auto button = std::make_shared<gui::Button>(L"Workshop", glm::vec4(10.f), [menu](gui::GUI*) {
		menu->setPage("workshop");
	});

	auto panel = std::dynamic_pointer_cast<gui::Panel>(menu->getNodes().front());
	if (!panel) return;
	std::vector<std::shared_ptr<gui::UINode>> nodes = panel->getNodes();
	panel->clear();
	for (size_t i = 0; i < nodes.size(); i++) {
		if (i == nodes.size() - 1) panel->add(button);
		panel->add(nodes[i]);
	}
	menu->setSize(panel->getSize());

	panel = std::make_shared<gui::Panel>(glm::vec2(400));
	menu->addPage("workshop", panel);

	std::vector<ContentPack> scanned;
	ContentPack::scanFolder(engine->getPaths()->getResources()/"content", scanned);

	for (const auto& pack : scanned) {
		panel->add(std::make_shared<gui::Button>(util::str2wstr_utf8(pack.id), glm::vec4(10.f), [engine, pack](gui::GUI*) {
			engine->setScreen(std::make_shared<workshop::WorkShopScreen>(engine, pack));
		}));
	}

	panel->add(std::make_shared<gui::Button>(L"Create new", glm::vec4(10.f), [engine, menu](gui::GUI*) {
		auto panel = std::make_shared<gui::Panel>(glm::vec2(400, 200));
		menu->addPage("new-content", panel);
		menu->setPage("new-content");
		fs::path path = engine->getPaths()->getResources() / "content";

		panel->add(std::make_shared<gui::Label>(L"Name"));
		auto nameInput = std::make_shared<gui::TextBox>(L"example_pack", glm::vec4(6.0f));
		nameInput->setTextValidator([=](const std::wstring& text) {
			std::string textutf8 = util::wstr2str_utf8(text);
			return util::is_valid_filename(text) &&
				!fs::exists(path / text) && !nameInput->getInput().empty();
		});
		panel->add(nameInput);

		panel->add(std::make_shared<gui::Button>(L"Create", glm::vec4(10.f), [=](gui::GUI*) {
			if (!nameInput->validate()) return;
			ContentPack pack;
			pack.folder = path / nameInput->getInput();
			pack.id = pack.title = util::wstr2str_utf8(nameInput->getInput());
			pack.version = "1.0";
			fs::create_directories(pack.folder / ContentPack::BLOCKS_FOLDER);
			fs::create_directories(pack.folder / ContentPack::ITEMS_FOLDER);
			fs::create_directories(pack.folder / TEXTURES_FOLDER);
			saveContentPack(pack);
			menu->remove(panel);

			engine->setScreen(std::make_shared<workshop::WorkShopScreen>(engine, pack));
		}));
		panel->add(std::make_shared<gui::Button>(L"Back", glm::vec4(10.f), [menu](gui::GUI*) {
			menu->back();
		}));
	}));
	panel->add(std::make_shared<gui::Button>(L"Back", glm::vec4(10.f), [menu](gui::GUI*) {
		menu->back();
	}));
}