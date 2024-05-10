#include "menu_workshop.hpp"

#include "../../engine.h"
#include "../../util/stringutil.h"
#include "../gui/gui_util.h"
#include "../menu/menu.h"
#include "../menu/menu_commons.h"
#include "../screens.h"
#include "WorkshopScreen.hpp"

void menus::create_workshop_panel(Engine* engine) {
	auto menu = engine->getGUI()->getMenu();
	auto panel = menus::create_page(engine, "workshop", 400, 0.0f, 1);

	std::vector<ContentPack> scanned;
	ContentPack::scan(engine->getPaths(), scanned);
	panel->add(menus::create_packs_panel(scanned, engine, false, [engine](const ContentPack& pack) {
		engine->setScreen(std::make_shared<workshop::WorkShopScreen>(engine, pack));
	}, packconsumer()));
	panel->add(menus::create_button(L"Create new", glm::vec4(10.f), glm::vec4(1.f), [engine](gui::GUI*) {
		auto newContent = menus::create_page(engine, "new-content", 400, 0.f, 1);
		auto menu = engine->getGUI()->getMenu();
		menu->setPage("new-content");
		fs::path path = engine->getPaths()->getResources() / "content";

		newContent->add(std::make_shared<gui::Label>(L"Name"));
		auto nameInput = std::make_shared<gui::TextBox>(L"example_pack", glm::vec4(6.0f));
		nameInput->setTextValidator([=](const std::wstring& text) {
			std::string textutf8 = util::wstr2str_utf8(text);
			return util::is_valid_filename(text) &&
				!fs::exists(path / text) && !nameInput->getInput().empty();
		});
		newContent->add(nameInput);

		newContent->add(menus::create_button(L"Create", glm::vec4(10.f), glm::vec4(1.f), [=](gui::GUI*) {
			if (!nameInput->validate()) return;
			ContentPack pack;
			pack.folder = path / nameInput->getInput();
			pack.id = pack.title = util::wstr2str_utf8(nameInput->getInput());
			pack.version = "1.0";
			fs::create_directories(pack.folder / ContentPack::BLOCKS_FOLDER);
			fs::create_directories(pack.folder / ContentPack::ITEMS_FOLDER);
			fs::create_directories(pack.folder / TEXTURES_FOLDER);
			ContentPack::write(pack);
			menu->remove(newContent);

			engine->setScreen(std::make_shared<workshop::WorkShopScreen>(engine, pack));
		}));
		newContent->add(guiutil::backButton(menu));
	}));
	panel->add(guiutil::backButton(menu));
}