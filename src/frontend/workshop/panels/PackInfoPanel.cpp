#include "frontend/workshop/WorkshopScreen.hpp"

#include "assets/Assets.hpp"
#include "coders/png.hpp"
#include "graphics/core/Texture.hpp"
#include "graphics/ui/elements/Panel.hpp"
#include "graphics/ui/elements/Button.hpp"
#include "graphics/ui/elements/Image.hpp"
#include "graphics/ui/elements/TextBox.hpp"
#include "frontend/workshop/gui_elements/BasicElements.hpp"
#include "frontend/workshop/WorkshopSerializer.hpp"
#include "engine/Engine.hpp"

#define NOMINMAX
#include "frontend/workshop/libs/portable-file-dialogs.h"

using namespace workshop;

static void createDependencyList(gui::Panel& dependencyList, gui::Panel& panel, ContentPack& pack, Engine& engine,
	std::vector<ContentPack> contentPacks) {
	dependencyList.clear();

	for (auto& elem : pack.dependencies) {
		const size_t dependencyIndex = &elem - pack.dependencies.data();

		gui::Container& container = *new gui::Container(glm::vec2(dependencyList.getSize().x));
		container.setScrollable(false);

		PacksManager manager = engine.createPacksManager(engine.getPaths().getCurrentWorldFolder());
		manager.scan();
		std::vector<ContentPack> scanned = manager.getAll(manager.getAllNames());

		gui::TextBox& textbox = createTextBox(container, elem.id, L"Dependency ID");
		textbox.setTooltipDelay(0.f);
		textbox.setTextValidator([scanned, &textbox, &pack, &elem](const std::wstring&) {
			const std::string input(util::wstr2str_utf8(textbox.getInput()));
			auto [code, isOk] = checkPackId(textbox.getInput(), scanned);
			bool packExist = std::find_if(scanned.begin(), scanned.end(), [input](const ContentPack& pack) {
				return pack.id == input;
			}) != scanned.end();
			if (elem.level == DependencyLevel::required) {
				code = packExist ? "Dependency exist" : "Dependency not exist";
				isOk = packExist;
			}
			else {
				if (!isOk && packExist) code = "Dependency available";
				else if (isOk) code = "Dependency not available";
				isOk = isOk || packExist;
			}
			if (input == pack.id){
				isOk = false;
				code = "Pack recursion";
			}
			textbox.setTooltip(util::str2wstr_utf8(code));
			return isOk;
		});

		const std::wstring levels[] = { L"required", L"optional", L"weak" };
		gui::Button& button = *new gui::Button(L"Level: " + levels[static_cast<int>(elem.level)], glm::vec4(10.f), gui::onaction());
		button.listenAction([&levelRef = pack.dependencies[dependencyIndex].level, levels, &button](gui::GUI*) {
			incrementEnumClass(levelRef, 1);
			if (levelRef > DependencyLevel::weak) levelRef = DependencyLevel::required;
			button.setText(L"Level: " + levels[static_cast<int>(levelRef)]);
		});
		gui::Image& image = *new gui::Image("gui/delete_icon");
		gui::Container& imageContainer = *new gui::Container(image.getSize());

		const float interval = textbox.getPadding().x + textbox.getMargin().x;
		const glm::vec2 size((panel.getSize().x - image.getSize().x) / 2.f - interval, textbox.getSize().y);
		textbox.setSize(size);
		button.setSize(size);
		button.setPos(glm::vec2(size.x + interval, 0.f));

		imageContainer.setPos(glm::vec2(size.x * 2.f + interval, 0.f));
		imageContainer.listenAction([=, &engine, &dependencyList, &panel, &pack](gui::GUI*) {
			pack.dependencies.erase(pack.dependencies.begin() + dependencyIndex);
			createDependencyList(dependencyList, panel, pack, engine, contentPacks);
			panel.refresh();
		});
		imageContainer << image;

		container << imageContainer;
		container << button;
		container.setSize(glm::vec2(container.getSize().x, size.y));

		dependencyList << container;
	}
	dependencyList.cropToContent();
}

void WorkShopScreen::createPackInfoPanel() {
	createPanel([this]() {

		auto loadIcon = [this]() {
			std::string icon = currentPackId + ".icon";
			Texture* iconTex = assets->get<Texture>(icon);
			if (iconTex == nullptr) {
				auto iconfile = currentPack.folder / fs::path("icon.png");
				if (fs::is_regular_file(iconfile)) {
					assets->store(png::load_texture(iconfile.string()), icon);
					return assets->get<Texture>(icon);
				}
				else {
					return assets->get<Texture>("gui/no_icon");
				}
			}
			return iconTex;
		};

		gui::Panel& panel = *new gui::Panel(glm::vec2(400));
		gui::Container& iconContainer = *new gui::Container(glm::vec2(panel.getSize().x, 64));
		gui::Image& iconImage = *new gui::Image(loadIcon(), glm::vec2(64));
		iconContainer << iconImage;
		gui::Button& setButton = *new gui::Button(L"Set icon", glm::vec4(10.f), [this, loadIcon, &iconImage](gui::GUI*) {
			auto newIcon = pfd::open_file("", "", { "(.png)", "*.png" }, pfd::opt::none).result();
			if (newIcon.empty()) return;
			fs::copy(newIcon.front(), currentPack.folder);
			std::string filename = fs::path(newIcon.front()).filename().string();
			fs::rename(currentPack.folder / filename, currentPack.folder / "icon.png");
			iconImage.setTexture(loadIcon());
		});
		const float posX = iconImage.getSize().x + 10.f;
		gui::Label& label = *new gui::Label("Recommended to use 128x128 png image");
		label.setPos(glm::vec2(posX, 5.f));
		iconContainer << label;
		setButton.setPos(glm::vec2(posX, label.calcPos().y + label.getSize().y));
		iconContainer << setButton;
		gui::Button& removeButton = *new gui::Button(L"Remove icon", glm::vec4(10.f), [this, loadIcon, &iconImage](gui::GUI*) {
			fs::path iconFile(currentPack.folder / "icon.png");
			if (fs::is_regular_file(iconFile)) {
				createFileDeletingConfirmationPanel({ iconFile }, 2, [this, &iconImage, loadIcon]() {
					assets->store(std::unique_ptr<Texture>(nullptr), currentPackId + ".icon");
					iconImage.setTexture(loadIcon());
				});
			}
		});
		removeButton.setPos(glm::vec2(setButton.calcPos().x + setButton.getSize().x + 10.f, label.calcPos().y + label.getSize().y));
		iconContainer << removeButton;
		panel << iconContainer;

		panel << new gui::Label("Creator");
		createTextBox(panel, currentPack.creator);
		panel << new gui::Label("Title");
		createTextBox(panel, currentPack.title, L"Example Pack");
		panel << new gui::Label("Version");
		createTextBox(panel, currentPack.version, L"1.0");
		panel << new gui::Label("ID");

		PacksManager manager = engine.createPacksManager(engine.getPaths().getCurrentWorldFolder());
		manager.scan();
		std::vector<ContentPack> scanned = manager.getAll(manager.getAllNames());

		gui::TextBox& id = createTextBox(panel, currentPack.id, L"example_pack");
		id.setTooltipDelay(0.f);
		id.setTextValidator([this, &id, scanned](const std::wstring&) {
			auto [code, isOk] = checkPackId(id.getInput(), scanned);
			if (util::wstr2str_utf8(id.getInput()) == currentPackId){
				code = "";
				isOk = true;
			}
			id.setTooltip(util::str2wstr_utf8(code));
			return isOk;
		});

		panel << new gui::Label("Description");
		createTextBox(panel, currentPack.description, L"My Example Pack");
		panel << new gui::Label("Dependencies");

		gui::Panel& dependencyList = *new gui::Panel(glm::vec2(0.f));
		dependencyList.setColor(glm::vec4(0.f));
		panel << dependencyList;

		createDependencyList(dependencyList, panel, currentPack, engine, scanned);

		panel << new gui::Button(L"Add dependency", glm::vec4(10.f), [this, &dependencyList, &panel, scanned](gui::GUI*) {
			currentPack.dependencies.emplace_back();
			createDependencyList(dependencyList, panel, currentPack, engine, scanned);
		});

		panel << new gui::Button(L"Save", glm::vec4(10.f), [this, &id](gui::GUI*) {
			if (!id.validate()) return;
			saveContentPack(currentPack);
		});

		return std::ref(panel);
	}, 1);
}