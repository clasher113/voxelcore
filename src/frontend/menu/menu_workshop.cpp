#include "menu.h"
#include "menu_commons.h"

#include "../../engine.h"
#include "../screens.h"
#include "../../util/stringutil.h"
#include "../gui/gui_util.h"
#include "../../coders/png.h"
#include "../gui/containers.h"

void menus::create_workshop_panel(Engine* engine) {
    auto menu = engine->getGUI()->getMenu();
    auto panel = menus::create_page(engine, "workshop", 400, 0.0f, 1);

    std::vector<ContentPack> scanned;
    ContentPack::scan(engine->getPaths(), scanned);
    panel->add(menus::create_packs_panel(scanned, engine, false, [engine](const ContentPack& pack) {
        engine->setScreen(std::make_shared<WorkShopScreen>(engine, pack));
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
            pack.version = std::to_string(ENGINE_VERSION_MAJOR) + "." + std::to_string(ENGINE_VERSION_MINOR);
            fs::create_directory(pack.folder);
            fs::create_directory(pack.folder / ContentPack::BLOCKS_FOLDER);
            fs::create_directory(pack.folder / ContentPack::ITEMS_FOLDER);
            fs::create_directory(pack.folder / TEXTURES_FOLDER);
            ContentPack::write(pack);
            menu->remove(newContent);

            engine->setScreen(std::make_shared<WorkShopScreen>(engine, pack));
            }));
        newContent->add(guiutil::backButton(menu));
        }));
    panel->add(guiutil::backButton(menu));
}