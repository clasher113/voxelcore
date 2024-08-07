#include "WorkshopScreen.hpp"

#include "../../coders/png.hpp"
#include "../../coders/xml.hpp"
#include "../../engine.hpp"
#include "../../files/files.hpp"
#include "../../graphics/core/Atlas.hpp"
#include "../../graphics/core/Shader.hpp"
#include "../../graphics/core/Texture.hpp"
#include "../../graphics/render/BlocksPreview.hpp"
#include "../../graphics/ui/elements/Menu.hpp"
#include "../../graphics/ui/GUI.hpp"
#include "../../items/ItemDef.hpp"
#include "../../util/stringutil.hpp"
#include "../../window/Camera.hpp"
#include "../../window/Events.hpp"
#include "../../window/Window.hpp"
#include "../../world/Level.hpp"
#include "../ContentGfxCache.hpp"
#include "IncludeCommons.hpp"
#include "menu_workshop.hpp"
#include "WorkshopPreview.hpp"
#include "WorkshopUtils.hpp"
#include "../../content/Content.hpp"
#include "../../audio/audio.hpp"

#define NOMINMAX
#include "libs/portable-file-dialogs.h"
#include "../screens/MenuScreen.hpp"

using namespace workshop;

WorkShopScreen::WorkShopScreen(Engine* engine, const ContentPack& pack) : Screen(engine),
	framerate(engine->getSettings().display.framerate.get()),
	gui(engine->getGUI()),
	currentPack(pack)
{
	gui->getMenu()->reset();
	uicamera.reset(new Camera(glm::vec3(), static_cast<float>(Window::height)));
	uicamera->perspective = false;
	uicamera->flipped = true;
	engine->getSettings().display.framerate = -1;

	if (initialize()) {
		gui->add(createNavigationPanel());
	}
}

WorkShopScreen::~WorkShopScreen() {
	for (const auto& elem : panels) {
		gui->remove(elem.second);
	}
}

void WorkShopScreen::update(float delta) {
	if (Events::jpressed(keycode::ESCAPE)) {
		if (panels.size() < 2) {
			exit();
		}
		else removePanel(panels.rbegin()->first);
		return;
	}

	for (const auto& elem : panels) {
		elem.second->act(delta);
		elem.second->setSize(glm::vec2(elem.second->getSize().x, Window::height - 4.f));
	}
	if (preview) {
		preview->lockedKeyboardInput = false;
		for (const auto& elem : panels) {
			if (hasFocusedTextbox(elem.second)) {
				preview->lockedKeyboardInput = true;
				break;
			}
		}
		preview->update(delta);
	}
}

void WorkShopScreen::draw(float delta) {
	Window::clear();
	Window::setBgColor(glm::vec3(0.2f));

	uicamera->setFov(static_cast<float>(Window::height));
	Shader* uishader = engine->getAssets()->get<Shader>("ui");
	uishader->use();
	uishader->uniformMatrix("u_projview", uicamera->getProjView());

	float width = static_cast<float>(Window::width), height = static_cast<float>(Window::height);

	batch->begin();
	batch->texture(engine->getAssets()->get<Texture>("gui/menubg"));
	batch->rect(0.f, 0.f,
		width, height, 0.f, 0.f, 0.f,
		UVRegion(0.f, 0.f, width / 64.f, height / 64.f),
		false, false, glm::vec4(1.0f));
	batch->flush();
}

bool WorkShopScreen::initialize() {
	auto& packs = engine->getContentPacks();
	packs.clear();
	packs.emplace_back(currentPack);

	std::vector<ContentPack> scanned;
	ContentPack::scanFolder(engine->getPaths()->getResources() / "content", scanned);

	if (currentPack.id != "base") {
		auto it = std::find_if(scanned.begin(), scanned.end(), [](const ContentPack& pack) {
			return pack.id == "base";
		});
		if (it != scanned.end()) packs.emplace_back(*it);
	}

	for (size_t i = 0; i < packs.size(); i++) {
		for (size_t j = 0; j < packs[i].dependencies.size(); j++) {
			if (std::find_if(scanned.begin(), scanned.end(), [&dependency = packs[i].dependencies[j]](const ContentPack& pack) {
				return dependency.id == pack.id;
			}) == scanned.end()) {
				createContentErrorMessage(packs[i], "Dependency \"" + packs[i].dependencies[j].id + "\" not found");
				return 0;
			}
		}
	}
	try {
		engine->loadContent();
	}
	catch (const contentpack_error& e) {
		createContentErrorMessage(*std::find_if(packs.begin(), packs.end(), [e](const ContentPack& pack) {
			return pack.id == e.getPackId();
			}), e.what());
		return 0;
	}
	catch (const std::exception& e) {
		createContentErrorMessage(packs.back(), e.what());
		return 0;
	}
	assets = engine->getAssets();
	content = engine->getContent();
	indices = content->getIndices();
	cache.reset(new ContentGfxCache(content, assets));
	previewAtlas = BlocksPreview::build(cache.get(), assets, content).release();
	assets->store(std::unique_ptr<Atlas>(previewAtlas), BLOCKS_PREVIEW_ATLAS);
	itemsAtlas = assets->get<Atlas>("items");
	blocksAtlas = assets->get<Atlas>("blocks");
	preview.reset(new Preview(engine, cache.get()));

	if (!fs::is_regular_file(currentPack.getContentFile()))
		return 1;
	blocksList.clear();
	itemsList.clear();
	auto contentList = files::read_json(currentPack.getContentFile());
	if (contentList->has("blocks")) {
		auto blocks = contentList->list("blocks");
		for (size_t i = 0; i < blocks->size(); i++) {
			blocksList.insert(blocks->str(i));
		}
	}
	if (contentList->has("items")) {
		auto items = contentList->list("items");
		for (size_t i = 0; i < items->size(); i++) {
			itemsList.insert(items->str(i));
		}
	}
	return 1;
}

void WorkShopScreen::exit() {
	Engine* e = engine;
	e->getSettings().display.framerate.set(framerate);
	e->setScreen(std::make_shared<MenuScreen>(e));
	create_workshop_button(e, &e->getGUI()->getMenu()->getCurrent());
	e->getGUI()->getMenu()->setPage("workshop");
}

std::shared_ptr<gui::Panel> WorkShopScreen::createNavigationPanel() {
	auto panel = std::make_shared<gui::Panel>(glm::vec2(250.f));

	panels.emplace(0, panel);
	panel->setPos(glm::vec2(2.f));
	panel->add(std::make_shared<gui::Button>(L"Info", glm::vec4(10.f), [this](gui::GUI*) {
		createPackInfoPanel();
	}));
	panel->add(std::make_shared<gui::Button>(L"Blocks", glm::vec4(10.f), [this](gui::GUI*) {
		createContentList(DefType::BLOCK);
	}));
	panel->add(std::make_shared<gui::Button>(L"Block Materials", glm::vec4(10.f), [this](gui::GUI*) {
		createMaterialsList();
	}));
	panel->add(std::make_shared<gui::Button>(L"Textures", glm::vec4(10.f), [this](gui::GUI*) {
		createTextureList(50.f);
	}));
	panel->add(std::make_shared<gui::Button>(L"Items", glm::vec4(10.f), [this](gui::GUI*) {
		createContentList(DefType::ITEM);
	}));
	panel->add(std::make_shared<gui::Button>(L"Sounds", glm::vec4(10.f), [this](gui::GUI*) {
		createSoundList();
	}));
	panel->add(std::make_shared<gui::Button>(L"Scripts", glm::vec4(10.f), [this](gui::GUI*) {
		createScriptList();
	}));
	panel->add(std::make_shared<gui::Button>(L"UI Layouts", glm::vec4(10.f), [this](gui::GUI*) {
		createUILayoutList();
	}));
	auto button = std::make_shared<gui::Button>(L"Back", glm::vec4(10.f), [this](gui::GUI*) {
		exit();
	});
	panel->add(button);

	setSelectable<gui::Button>(panel);
	createPackInfoPanel();

	auto container = std::make_shared<gui::Container>(panel->getSize());
	container->listenInterval(0.1f, [panel, button, container]() {
		container->setSize(glm::vec2(panel->getSize().x, Window::height - (button->calcPos().y + (button->getSize().y + 10.f) * 2)));
	});
	panel->add(container);

	panel->add(button = std::make_shared<gui::Button>(L"Open Pack Folder", glm::vec4(10.f), [this](gui::GUI*) {
		openPath(currentPack.folder);
	}));

	return panel;
}

void WorkShopScreen::createContentErrorMessage(ContentPack& pack, const std::string& message) {
	auto panel = std::make_shared<gui::Panel>(glm::vec2(500.f, 20.f));
	panel->listenInterval(0.1f, [panel]() {
		panel->setPos(Window::size() / 2.f - panel->getSize() / 2.f);
	});

	panel->add(std::make_shared<gui::Label>("Error in content pack \"" + pack.id + "\":"));
	auto label = std::make_shared<gui::Label>(message);
	label->setMultiline(true);
	panel->add(label);

	panel->add(std::make_shared<gui::Button>(L"Open pack folder", glm::vec4(10.f), [pack](gui::GUI*) {
		openPath(pack.folder);
	}));
	panel->add(std::make_shared<gui::Button>(L"Back", glm::vec4(10.f), [this, panel](gui::GUI*) { gui->remove(panel); exit(); }));

	panel->setPos(Window::size() / 2.f - panel->getSize() / 2.f);
	gui->add(panel);
}

void WorkShopScreen::removePanel(unsigned int column) {
	gui->remove(panels[column]);
	panels.erase(column);
}

void WorkShopScreen::removePanels(unsigned int column) {

	for (auto it = panels.begin(); it != panels.end();) {
		if (it->first >= column) {
			gui->remove(it->second);
			panels.erase(it++);
		}
		else ++it;
	}
}

void WorkShopScreen::clearRemoveList(std::shared_ptr<gui::Panel> panel) {
	removeList.erase(std::remove_if(removeList.begin(), removeList.end(), [panel](std::shared_ptr<gui::UINode> node) {
		for (const auto& elem : panel->getNodes()) {
			if (elem == node) {
				panel->remove(elem);
				return true;
			}
		}
		return false;
	}), removeList.end());
}

void WorkShopScreen::createPanel(std::function<std::shared_ptr<gui::Panel>()> lambda, unsigned int column, float posX) {
	removePanels(column);

	auto panel = lambda();
	panels.emplace(column, panel);

	if (posX == PANEL_POSITION_AUTO) {
		float x = 2.f;
		for (const auto& elem : panels) {
			elem.second->setPos(glm::vec2(x, 2.f));
			x += elem.second->getSize().x;
		}
	}
	else {
		panel->setPos(glm::vec2(posX, 2.f));
	}
	gui->add(panel);
}

std::shared_ptr<gui::Panel> WorkShopScreen::createTexturesPanel(std::shared_ptr<gui::Panel> panel, float iconSize, std::string* textures, BlockModel model) {
	panel->setColor(glm::vec4(0.f));

	const char* faces[] = { "East:", "West:", "Bottom:", "Top:", "South:", "North:" };

	size_t buttonsNum = 0;

	switch (model) {
	case BlockModel::block:
	case BlockModel::aabb: buttonsNum = 6;
		break;
	case BlockModel::custom:
	case BlockModel::xsprite: buttonsNum = 1;
		break;
	}
	if (buttonsNum == 0) return panel;
	panel->add(removeList.emplace_back(std::make_shared<gui::Label>(buttonsNum == 1 ? L"Texture" : L"Texture faces")));
	for (size_t i = 0; i < buttonsNum; i++) {
		auto button = std::make_shared<gui::IconButton>(glm::vec2(panel->getSize().x, iconSize), textures[i], blocksAtlas, textures[i],
			(buttonsNum == 6 ? faces[i] : ""));
		button->listenAction([this, button, model, textures, iconSize, i](gui::GUI*) {
			createTextureList(35.f, 5, DefType::BLOCK, button->calcPos().x + button->getSize().x, true,
			[this, button, model, textures, iconSize, i](const std::string& texName) {
					textures[i] = getTexName(texName);
					removePanel(5);
					button->setColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.95f));
					button->setHoverColor(glm::vec4(0.05f, 0.1f, 0.15f, 0.75f));
					button->setIcon(getAtlas(assets, texName), getTexName(texName));
					button->setText(getTexName(texName));
					preview->updateCache();
			});
		});
		panel->add(removeList.emplace_back(button));
	}

	setSelectable<gui::IconButton>(panel);

	return panel;
}

std::shared_ptr<gui::Panel> WorkShopScreen::createTexturesPanel(std::shared_ptr<gui::Panel> panel, float iconSize, std::string& texture, item_icon_type iconType) {
	panel->setColor(glm::vec4(0.f));
	if (iconType == item_icon_type::none) return panel;

	auto texName = [this, iconType, &texture]() {
		if (iconType == item_icon_type::sprite) return getTexName(texture);
		return texture;
	};

	auto button = std::make_shared<gui::IconButton>(glm::vec2(panel->getSize().x, iconSize), texName(), getAtlas(assets, texture), texName());
	button->listenAction([this, button, texName, panel, &texture, iconSize, iconType](gui::GUI*) {
		auto& nodes = button->getNodes();
		auto callback = [this,button, nodes, texName, iconSize, &texture](const std::string& textureName) {
			texture = textureName;
			removePanel(5);
			button->setIcon(getAtlas(assets, texture), texName());
			button->setText(texName());
		};
		if (iconType == item_icon_type::sprite) {
			createTextureList(35.f, 5, DefType::BOTH, PANEL_POSITION_AUTO, true, callback);
		}
		else {
			createContentList(DefType::BLOCK, 5, true, callback);
		}
		button->setColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.95f));
		button->setHoverColor(glm::vec4(0.05f, 0.1f, 0.15f, 0.75f));
	});
	panel->add(removeList.emplace_back(button));

	return panel;
}

void WorkShopScreen::createAddingUIElementPanel(float posX, const std::function<void(const std::string&)>& callback, unsigned int filter) {
	createPanel([this, callback, filter]() {
		auto panel = std::make_shared<gui::Panel>(glm::vec2(200.f));

		for (const auto& elem : uiElementsArgs) {
			if (elem.second.args & UIElementsArgs::INVENTORY || filter & elem.second.args) continue;
			const std::string& name(elem.first);
			panel->add(std::make_shared<gui::Button>(util::str2wstr_utf8(name), glm::vec4(10.f), [this, callback, &name](gui::GUI*) {
				callback(name);
			}));
		}
		return panel;
	}, 5, posX);
}

void WorkShopScreen::createMaterialsList(bool showAll, unsigned int column, float posX, const std::function<void(const std::string&)>& callback) {
	createPanel([this, showAll, callback]() {
		auto panel = std::make_shared<gui::Panel>(glm::vec2(200));

		for (auto& elem : content->getBlockMaterials()) {
			if (!showAll && elem.first.substr(0, currentPack.id.length()) != currentPack.id) continue;
			auto button = std::make_shared<gui::Button>(util::str2wstr_utf8(elem.first), glm::vec4(10.f), [this, &elem, callback](gui::GUI*) {
				if (callback) callback(elem.first);
				else createMaterialEditor(const_cast<BlockMaterial&>(*elem.second));
			});
			button->setTextAlign(gui::Align::left);
			panel->add(button);
		}

		return panel;
	}, column, posX);
}

void WorkShopScreen::createScriptList(unsigned int column, float posX, const std::function<void(const std::string&)>& callback) {
	createPanel([this, callback]() {
		auto panel = std::make_shared<gui::Panel>(glm::vec2(200));

		fs::path scriptsFolder(currentPack.folder / "scripts/");
		std::set<fs::path> scripts = getFiles(scriptsFolder, true);
		if (callback) scripts.insert(fs::path());

		for (const auto& elem : scripts) {
			fs::path parentDir(fs::relative(elem.parent_path(), scriptsFolder));
			auto button = std::make_shared<gui::Button>(util::str2wstr_utf8(getScriptName(currentPack, parentDir.string() + "/" + elem.stem().string())), glm::vec4(10.f),
				[this, elem, callback](gui::GUI*) {
					if (callback) callback(elem.stem().string());
					else createScriptInfoPanel(elem);
				});
			button->setTextAlign(gui::Align::left);
			panel->add(button);
		}

		setSelectable<gui::Button>(panel);

		return panel;
	}, column, posX);
}

void WorkShopScreen::createScriptInfoPanel(const fs::path& file) {
	createPanel([this, file]() {
		auto panel = std::make_shared<gui::Panel>(glm::vec2(400));

		std::string fileName(file.stem().string());
		std::string extention(file.extension().string());

		panel->add(std::make_shared<gui::Label>("Path: " + fs::relative(file, currentPack.folder).remove_filename().string()));
		panel->add(std::make_shared<gui::Label>("File: " + fileName + extention));

		panel->add(std::make_shared<gui::Button>(L"Open script", glm::vec4(10.f), [file](gui::GUI*) {
			openPath(file);
		}));

		return panel;
	}, 2);
}

void WorkShopScreen::createSoundList() {
	createPanel([this]() {
		auto panel = std::make_shared<gui::Panel>(glm::vec2(200));

		std::set<fs::path> sounds = getFiles(currentPack.folder / SOUNDS_FOLDER, true);

		for (const auto& elem : sounds) {
			auto button = std::make_shared<gui::Button>(util::str2wstr_utf8(elem.stem().string()), glm::vec4(10.f), [this, elem](gui::GUI*) {
				createSoundInfoPanel(elem);
			});
			button->setTextAlign(gui::Align::left);
			panel->add(button);
		}

		return panel;
	}, 1);
}

void WorkShopScreen::createSoundInfoPanel(const fs::path& file) {
	createPanel([this, file]() {
		auto panel = std::make_shared<gui::Panel>(glm::vec2(400));

		std::string fileName(file.stem().string());
		std::string extention(file.extension().string());

		panel->add(std::make_shared<gui::Label>("Path: " + fs::relative(file, currentPack.folder).remove_filename().string()));
		panel->add(std::make_shared<gui::Label>("File: " + fileName + extention));

		fs::path parentDir = fs::relative(file.parent_path(), currentPack.folder / SOUNDS_FOLDER);

		//audio::Sound* sound = assets->get<audio::Sound>(parentDir.empty() ? "" : parentDir.string() + "/" + fileName);
  //      sound->newInstance(0, 0);

		return panel;
	}, 2);
}

void WorkShopScreen::createTextureInfoPanel(const std::string& texName, DefType type) {
	createPanel([this, texName, type]() {
		auto panel = std::make_shared<gui::Panel>(glm::vec2(350));

		panel->add(std::make_shared<gui::Label>(texName));
		auto imageContainer = std::make_shared<gui::Container>(glm::vec2(panel->getSize().x));
		Atlas* atlas = getAtlas(assets, texName);
		Texture* tex = atlas->getTexture();
		auto image = std::make_shared<gui::Image>(tex, glm::vec2(0.f));
		formatTextureImage(*image, atlas, panel->getSize().x, getTexName(texName));
		imageContainer->add(image);
		panel->add(imageContainer);
		const UVRegion& uv = atlas->get(getTexName(texName));
		glm::ivec2 size((uv.u2 - uv.u1) * tex->getWidth(), (uv.v2 - uv.v1) * tex->getHeight());
		panel->add(std::make_shared<gui::Label>(L"Width: " + std::to_wstring(size.x)));
		panel->add(std::make_shared<gui::Label>(L"Height: " + std::to_wstring(size.y)));
		panel->add(std::make_shared<gui::Label>(L"Texture type: " + util::str2wstr_utf8(getDefName(type))));
		panel->add(std::make_shared<gui::Button>(L"Delete", glm::vec4(10.f), [this, texName, type](gui::GUI*) {
			createFileDeletingConfirmationPanel(currentPack.folder / TEXTURES_FOLDER / getDefFolder(type) / std::string(getTexName(texName) + ".png"),
			3, [this]() {
					initialize();
					createTextureList(50.f, 1);
				});
			}));
		return panel;
	}, 2);
}

void WorkShopScreen::createImportPanel(DefType type, std::string mode) {
	createPanel([this, type, mode]() {
		auto panel = std::make_shared<gui::Panel>(glm::vec2(200));

		panel->add(std::make_shared<gui::Label>(L"Import texture"));
		panel->add(std::make_shared<gui::Button>(L"Import as: " + util::str2wstr_utf8(getDefName(type)), glm::vec4(10.f), [this, type, mode](gui::GUI*) {
			switch (type) {
			case DefType::BLOCK: createImportPanel(DefType::ITEM, mode); break;
			case DefType::ITEM: createImportPanel(DefType::BLOCK, mode); break;
			}
		}));
		panel->add(std::make_shared<gui::Button>(L"Import mode: " + util::str2wstr_utf8(mode), glm::vec4(10.f), [this, type, mode](gui::GUI*) {
			if (mode == "copy") createImportPanel(type, "move");
			else if (mode == "move") createImportPanel(type, "copy");
		}));
		panel->add(std::make_shared<gui::Button>(L"Select files", glm::vec4(10.f), [this, type, mode](gui::GUI*) {
			auto files = pfd::open_file("", "", { "(.png)", "*.png" }, pfd::opt::multiselect).result();
			if (files.empty()) return;
			fs::path path(currentPack.folder / TEXTURES_FOLDER / getDefFolder(type));
			if (!fs::is_directory(path)) fs::create_directories(path);
			for (const auto& elem : files) {
				if (fs::is_regular_file(path / fs::path(elem).filename())) continue;
				fs::copy(elem, path);
				if (mode == "move") fs::remove(elem);
			}
			initialize();
			createTextureList(50.f);
		}));

		return panel;
	}, 2);
}

void workshop::WorkShopScreen::createFileDeletingConfirmationPanel(const std::filesystem::path& file, unsigned int column,
	const std::function<void(void)>& callback)
{
	createPanel([this, file, column, callback]() {
		auto panel = std::make_shared<gui::Panel>(glm::vec2(200));

		auto label = std::make_shared<gui::Label>("Are you sure you want to permanently delete this file?" + file.string());
		//label->setTextWrapping(false);
		//label->setMultiline(true);
		//label->setVerticalAlign(gui::Align::top);
		//label->setSize(glm::vec2(panel->getSize().x, 400));
		panel->add(label);

		panel->add(std::make_shared<gui::Button>(L"Confirm", glm::vec4(10.f), [this, file, column, callback](gui::GUI*) {
			std::filesystem::remove(file);
			removePanel(column);
			if (callback) callback();
		}));
		panel->add(std::make_shared<gui::Button>(L"Cancel", glm::vec4(10.f), [this, column](gui::GUI*) {
			removePanel(column);
		}));

		return panel;
	}, column);
}

void WorkShopScreen::createMaterialEditor(BlockMaterial& material) {
	createPanel([this, &material]() {
		auto panel = std::make_shared<gui::Panel>(glm::vec2(200));

		panel->add(std::make_shared<gui::Label>(material.name));

		panel->add(std::make_shared<gui::Label>("Step Sound"));
		createTextBox(panel, material.stepsSound);
		panel->add(std::make_shared<gui::Label>("Place Sound"));
		createTextBox(panel, material.placeSound);
		panel->add(std::make_shared<gui::Label>("Break Sound"));
		createTextBox(panel, material.breakSound);

		return panel;
	}, 2);
}

void WorkShopScreen::createBlockPreview(unsigned int column, PrimitiveType type) {
	createPanel([this, type]() {
		auto panel = std::make_shared<gui::Panel>(glm::vec2(300));
		auto image = std::make_shared<gui::Image>(preview->getTexture(), glm::vec2(panel->getSize().x));
		panel->add(image);
		panel->listenInterval(0.01f, [this, panel, image]() {
			if (panel->isHover() && image->isInside(Events::cursor)) {
				if (Events::jclicked(mousecode::BUTTON_1)) preview->mouseLocked = true;
				if (Events::scroll) preview->scale(-Events::scroll / 10.f);
			}
			panel->setSize(glm::vec2(Window::width - panel->calcPos().x - 2.f, Window::height));
			image->setSize(glm::vec2(image->getSize().x, Window::height / 2.f));
			preview->drawBlock();
			preview->setResolution(static_cast<uint>(image->getSize().x), static_cast<uint>(image->getSize().y));
			image->setTexture(preview->getTexture());
		});
		createFullCheckBox(panel, L"Draw grid", preview->drawGrid);
		createFullCheckBox(panel, L"Show front direction", preview->drawDirection);
		createFullCheckBox(panel, L"Draw block size", preview->drawBlockSize);
		if (type == PrimitiveType::HITBOX)
			createFullCheckBox(panel, L"Draw current hitbox", preview->drawBlockHitbox);
		else if (type == PrimitiveType::AABB)
			createFullCheckBox(panel, L"Highlight current AABB", preview->drawCurrentAABB);
		else if (type == PrimitiveType::TETRAGON)
			createFullCheckBox(panel, L"Highlight current Tetragon", preview->drawCurrentTetragon);
		panel->add(std::make_shared<gui::Label>(""));
		panel->add(std::make_shared<gui::Label>("Rotate: W,S,A,D or mouse"));
		panel->add(std::make_shared<gui::Label>("Zoom: Left Shift/Space or Scroll Wheel"));

		return panel;
	}, column);
}

void WorkShopScreen::createUIPreview() {
	createPanel([this]() {
		auto panel = std::make_shared<gui::Panel>(glm::vec2(300));
		auto image = std::make_shared<gui::Image>(preview->getTexture(), glm::vec2(panel->getSize().x, Window::height));
		panel->add(image);
		panel->listenInterval(0.01f, [this, panel, image]() {
			panel->setSize(glm::vec2(Window::width - panel->calcPos().x - 2.f, Window::height));
			image->setSize(glm::vec2(image->getSize().x, Window::height));
			preview->setResolution(Window::width, Window::height);
			image->setTexture(preview->getTexture());
			image->setUVRegion(UVRegion(0.f, 0.f, image->getSize().x / Window::width, 1.f));
			preview->drawUI();
		});

		return panel;
	}, 3);
}