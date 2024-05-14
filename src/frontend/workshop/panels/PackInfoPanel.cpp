#include "../WorkshopScreen.hpp"

#include "../../../assets/Assets.hpp"
#include "../../../coders/png.hpp"
#include "../../../graphics/core/Texture.hpp"
#include "../IncludeCommons.hpp"
#include "../libs/portable-file-dialogs.h"
#include "../WorkshopSerializer.hpp"

namespace workshop {

	static std::shared_ptr<gui::Panel> createDependencyList(std::shared_ptr<gui::Panel> panel, ContentPack& pack, Engine* engine) {
		panel->clear();

		for (auto& elem : pack.dependencies) {
			size_t dependencyIndex = &elem - pack.dependencies.data();

			auto container = std::make_shared<gui::Container>(glm::vec2(panel->getSize().x));
			container->setScrollable(false);

			std::vector<ContentPack> scanned;
			ContentPack::scanFolder(engine->getPaths()->getResources() / "content", scanned);

			auto textbox = createTextBox(container, elem.id, L"Dependency ID");
			std::unordered_map<DependencyLevel, std::wstring> levels = {
				{ DependencyLevel::required, L"required" },
				{ DependencyLevel::optional, L"optional" },
				{ DependencyLevel::weak, L"weak" }
			};
			textbox->setTextValidator([scanned, textbox, &pack](const std::wstring&) {
				const std::string input(util::wstr2str_utf8(textbox->getInput()));
				return std::find_if(scanned.begin(), scanned.end(), [input](const ContentPack& pack) {
					return pack.id == input;
				}) != scanned.end() && input != pack.id;
			});

			auto button = std::make_shared<gui::Button>(L"Level: " + levels.at(elem.level), glm::vec4(10.f), gui::onaction());
			//button->listenAction([&dependencies, dependencyIndex, levels, button](gui::GUI*) {
			//	DependencyLevel& level = dependencies[dependencyIndex].level;
			//	size_t index = static_cast<size_t>(level) + 1;
			//	if (index >= levels.size()) index = 0;
			//	level = static_cast<DependencyLevel>(index);
			//	button->setText(L"Level: " + levels.at(level));
			//});
			auto image = std::make_shared<gui::Image>(engine->getAssets()->getTexture("gui/delete_icon"));
			auto imageContainer = std::make_shared<gui::Container>(image->getSize());

			float interval = textbox->getPadding().x + textbox->getMargin().x;
			glm::vec2 size((panel->getSize().x - image->getSize().x) / 2.f - interval, textbox->getSize().y);
			textbox->setSize(size);
			button->setSize(size);
			button->setPos(glm::vec2(size.x + interval, 0.f));

			imageContainer->setHoverColor(textbox->getHoverColor());
			imageContainer->setPos(glm::vec2(size.x * 2.f + interval, 0.f));
			imageContainer->setScrollable(false);
			imageContainer->listenAction([engine, panel, &pack, dependencyIndex](gui::GUI*) {
				pack.dependencies.erase(pack.dependencies.begin() + dependencyIndex);
				createDependencyList(panel, pack, engine);
			});
			imageContainer->add(image);

			container->add(imageContainer);
			container->add(button);
			container->setSize(glm::vec2(container->getSize().x, size.y));

			panel->add(container);
		}
		panel->cropToContent();
		return panel;
	}
}

void workshop::WorkShopScreen::createPackInfoPanel() {
	createPanel([this]() {

		auto loadIcon = [this]() {
			std::string icon = currentPack.id + ".icon";
			Texture* iconTex = assets->getTexture(icon);
			if (iconTex == nullptr) {
				auto iconfile = currentPack.folder / fs::path("icon.png");
				if (fs::is_regular_file(iconfile)) {
					assets->store(png::load_texture(iconfile.string()).release(), icon);
					return assets->getTexture(icon);
				}
				else {
					return assets->getTexture("gui/no_icon");
				}
			}
			return iconTex;
		};

		auto panel = std::make_shared<gui::Panel>(glm::vec2(400));
		auto iconContainer = std::make_shared<gui::Container>(glm::vec2(panel->getSize().x, 64));
		auto iconImage = std::make_shared<gui::Image>(loadIcon(), glm::vec2(64));
		iconContainer->add(iconImage);
		auto setButton = std::make_shared<gui::Button>(L"Set icon", glm::vec4(10.f), [this, loadIcon, iconImage](gui::GUI*) {
			auto newIcon = pfd::open_file("", "", { "(.png)", "*.png" }, pfd::opt::none).result();
			if (newIcon.empty()) return;
			fs::copy(newIcon.front(), currentPack.folder);
			std::string filename = fs::path(newIcon.front()).filename().string();
			fs::rename(currentPack.folder / filename, currentPack.folder / "icon.png");
			iconImage->setTexture(loadIcon());
		});
		float posX = iconImage->getSize().x + 10.f;
		auto label = std::make_shared<gui::Label>("Recommended to use 128x128 png image");
		label->setPos(glm::vec2(posX, 5.f));
		iconContainer->add(label);
		setButton->setPos(glm::vec2(posX, label->calcPos().y + label->getSize().y));
		iconContainer->add(setButton);
		auto removeButton = std::make_shared<gui::Button>(L"Remove icon", glm::vec4(10.f), [this, loadIcon, iconImage](gui::GUI*) {
			fs::path iconFile(currentPack.folder / "icon.png");
			if (fs::is_regular_file(iconFile)) {
				createFileDeletingConfirmationPanel(iconFile, 2, [this, iconImage, loadIcon]() {
					assets->store(static_cast<Texture*>(nullptr), currentPack.id + ".icon");
					iconImage->setTexture(loadIcon());
				});
			}
		});
		removeButton->setPos(glm::vec2(setButton->calcPos().x + setButton->getSize().x + 10.f, label->calcPos().y + label->getSize().y));
		iconContainer->add(removeButton);
		panel->add(iconContainer);

		panel->add(std::make_shared<gui::Label>("Creator"));
		createTextBox(panel, currentPack.creator);
		panel->add(std::make_shared<gui::Label>("Title"));
		createTextBox(panel, currentPack.title, L"Example Pack");
		panel->add(std::make_shared<gui::Label>("Version"));
		createTextBox(panel, currentPack.version, L"1.0");
		panel->add(std::make_shared<gui::Label>("ID"));
		std::vector<ContentPack> scanned;
		ContentPack::scanFolder(engine->getPaths()->getResources() / "content", scanned);
		auto id = createTextBox(panel, currentPack.id, L"example_pack");
		id->setTextValidator([this, id, scanned](const std::wstring&) {
			std::wstring input(id->getInput());
			return !(input.length() < 2 || input.length() > 24 || isdigit(input[0]) ||
				!std::all_of(input.begin(), input.end(), [](const wchar_t c) { return c < 255 && (iswalnum(c) || c == '_'); }) ||
				std::find(ContentPack::RESERVED_NAMES.begin(), ContentPack::RESERVED_NAMES.end(), util::wstr2str_utf8(input)) != ContentPack::RESERVED_NAMES.end() ||
				std::find_if(scanned.begin(), scanned.end(), [this, input](const ContentPack& pack) { 
					return util::wstr2str_utf8(input) == pack.id && pack.id != currentPack.id; }
				) != scanned.end()
			);
		});

		panel->add(std::make_shared<gui::Label>("Description"));
		createTextBox(panel, currentPack.description, L"My Example Pack");
		panel->add(std::make_shared<gui::Label>("Dependencies"));

		auto dependencyList = std::make_shared<gui::Panel>(glm::vec2(0.f));
		dependencyList->setColor(glm::vec4(0.f));

		createDependencyList(dependencyList, currentPack, engine);

		panel->add(dependencyList);

		panel->add(std::make_shared<gui::Button>(L"Add dependency", glm::vec4(10.f), [this, dependencyList](gui::GUI*) {
			currentPack.dependencies.emplace_back();
			createDependencyList(dependencyList, currentPack, engine);
		}));

		panel->add(std::make_shared<gui::Button>(L"Save", glm::vec4(10.f), [this, id](gui::GUI*) {
			if (!id->validate()) return;
			saveContentPack(currentPack);
		}));

		return panel;
	}, 1);
}