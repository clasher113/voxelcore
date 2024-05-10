#include "../WorkshopScreen.hpp"

#include "../../../assets/Assets.h"
#include "../../../coders/png.h"
#include "../IncludeCommons.hpp"
#include "../libs/portable-file-dialogs.h"

void workshop::WorkShopScreen::createPackInfoPanel() {
	createPanel([this]() {

		auto loadIcon = [this]() {
			std::string icon = currentPack.id + ".icon";
			Texture* iconTex = assets->getTexture(icon);
			if (iconTex == nullptr) {
				auto iconfile = currentPack.folder / fs::path("icon.png");
				if (fs::is_regular_file(iconfile)) {
					Assets l_assets;
					l_assets.store(png::load_texture(iconfile.string()), icon);
					assets->extend(l_assets);
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
					Assets l_assets;
					l_assets.store(static_cast<Texture*>(nullptr), currentPack.id + ".icon");
					assets->extend(l_assets);
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
		createTextBox(panel, currentPack.id, L"example_pack");
		panel->add(std::make_shared<gui::Label>("Description"));
		createTextBox(panel, currentPack.description, L"My Example Pack");
		panel->add(std::make_shared<gui::Label>("Dependencies"));

		auto depencyList = std::make_shared<gui::Panel>(glm::vec2(0.f));
		depencyList->setColor(glm::vec4(0.f));
		auto createDepencyList = [this, depencyList]() {
			for (auto& elem : currentPack.dependencies) {
				removeList.emplace_back(createTextBox(depencyList, elem));
			}
			depencyList->cropToContent();
		};
		createDepencyList();
		panel->add(depencyList);

		panel->add(std::make_shared<gui::Button>(L"Add depency", glm::vec4(10.f), [this, createDepencyList, depencyList](gui::GUI*) {
			clearRemoveList(depencyList);
			currentPack.dependencies.emplace_back("");
			createDepencyList();
		}));

		panel->add(std::make_shared<gui::Button>(L"Save", glm::vec4(10.f), [this](gui::GUI*) {
			ContentPack::write(currentPack);
		}));

		return panel;
	}, 1);
}