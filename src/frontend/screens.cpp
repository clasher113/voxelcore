#include "screens.h"

#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <glm/glm.hpp>
#include <filesystem>
#include <stdexcept>

#include "../audio/audio.h"
#include "../window/Camera.h"
#include "../window/Events.h"
#include "../window/input.h"
#include "../graphics/Shader.h"
#include "../graphics/Batch2D.h"
#include "../graphics/GfxContext.h"
#include "../graphics/TextureAnimation.h"
#include "../assets/Assets.h"
#include "../world/Level.h"
#include "../world/World.h"
#include "../objects/Player.h"
#include "../physics/Hitbox.h"
#include "../logic/ChunksController.h"
#include "../logic/LevelController.h"
#include "../logic/scripting/scripting.h"
#include "../logic/scripting/scripting_frontend.h"
#include "../voxels/Chunks.h"
#include "../voxels/Chunk.h"
#include "../engine.h"
#include "../util/stringutil.h"
#include "../core_defs.h"
#include "WorldRenderer.h"
#include "hud.h"
#include "ContentGfxCache.h"
#include "LevelFrontend.h"
#include "gui/GUI.h"
#include "gui/containers.h"
#include "menu/menu.h"

#include "../content/Content.h"
#include "../voxels/Block.h"

#include "gui/controls.h"
#include "../content/ContentLoader.h"
#include "../coders/json.h"
#include "../data/dynamic.h"
#include "../files/files.h"
#include "../coders/png.h"
#include "BlocksPreview.h"
#include "../frontend/graphics/BlocksRenderer.h"
#include "../graphics/Atlas.h"
#include "../graphics/Mesh.h"
#include "../graphics/Texture.h"
#include "../graphics/LineBatch.h"
#include "../graphics/Framebuffer.h"
#include "../items/ItemDef.h"
#include "../voxels/ChunksStorage.h"
#include "locale/langs.h"

#ifdef _WIN32
#define NOMINMAX
#include <shlobj_core.h>
#include <wrl/client.h>
#undef DELETE 
#undef CREATE_NEW

HRESULT MultiselectInvoke(std::vector<fs::path>& files) {
	IFileOpenDialog* pfd;

	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&pfd));

	DWORD dwOptions;
	if (FAILED(hr = pfd->GetOptions(&dwOptions))) return hr;
	if (FAILED(hr = pfd->SetOptions(dwOptions | FOS_ALLOWMULTISELECT))) return hr;

	COMDLG_FILTERSPEC rgSpec[] = { { L"", L"*.png"} };
	if (FAILED(hr = pfd->SetFileTypes(ARRAYSIZE(rgSpec), rgSpec))) return hr;
	if (FAILED(hr = pfd->SetDefaultExtension(L"png"))) return hr;
	if (FAILED(hr = pfd->Show(NULL))) return hr;

	IShellItemArray* psiaResults;
	if (FAILED(hr = pfd->GetResults(&psiaResults))) return hr;
	DWORD cItems;
	if (FAILED(hr = psiaResults->GetCount(&cItems))) return hr;

	for (UINT i = 0; i < cItems; ++i) {
		Microsoft::WRL::ComPtr<IShellItem> pShellItem;
		if (FAILED(hr = psiaResults->GetItemAt(i, &pShellItem))) continue;
		WCHAR* name = nullptr;
		pShellItem->GetDisplayName(SIGDN_FILESYSPATH, &name);
		files.emplace_back(name);
	}

	psiaResults->Release();
	pfd->Release();

	return hr;
}
#endif // _WIN32

Screen::Screen(Engine* engine) : engine(engine), batch(new Batch2D(1024)) {
}

Screen::~Screen() {
}

MenuScreen::MenuScreen(Engine* engine_) : Screen(engine_) {
    auto menu = engine->getGUI()->getMenu();
    menus::refresh_menus(engine);
    menu->reset();
    menu->setPage("main");

    uicamera.reset(new Camera(glm::vec3(), Window::height));
    uicamera->perspective = false;
    uicamera->flipped = true;
}

MenuScreen::~MenuScreen() {
}

void MenuScreen::update(float delta) {
}

void MenuScreen::draw(float delta) {
    Window::clear();
    Window::setBgColor(glm::vec3(0.2f));

    uicamera->setFov(Window::height);
    Shader* uishader = engine->getAssets()->getShader("ui");
    uishader->use();
    uishader->uniformMatrix("u_projview", uicamera->getProjView());

    uint width = Window::width;
    uint height = Window::height;

    batch->begin();
    batch->texture(engine->getAssets()->getTexture("gui/menubg"));
    batch->rect(
        0, 0, 
        width, height, 0, 0, 0, 
        UVRegion(0, 0, width/64, height/64), 
        false, false, glm::vec4(1.0f)
    );
    batch->flush();
}

static bool backlight;

LevelScreen::LevelScreen(Engine* engine, Level* level) : Screen(engine) {
    auto& settings = engine->getSettings();
    auto assets = engine->getAssets();
    auto menu = engine->getGUI()->getMenu();
    menu->reset();

    controller = std::make_unique<LevelController>(settings, level);
    frontend = std::make_unique<LevelFrontend>(controller.get(), assets);

    worldRenderer = std::make_unique<WorldRenderer>(engine, frontend.get(), controller->getPlayer());
    hud = std::make_unique<Hud>(engine, frontend.get(), controller->getPlayer());
    

    backlight = settings.graphics.backlight;

    animator = std::make_unique<TextureAnimator>();
    animator->addAnimations(assets->getAnimations());

    auto content = level->content;
    for (auto& entry : content->getPacks()) {
        auto pack = entry.second.get();
        const ContentPack& info = pack->getInfo();
        fs::path scriptFile = info.folder/fs::path("scripts/hud.lua");
        if (fs::is_regular_file(scriptFile)) {
            scripting::load_hud_script(pack->getEnvironment()->getId(), info.id, scriptFile);
        }
    }
    scripting::on_frontend_init(hud.get());
}

LevelScreen::~LevelScreen() {
    scripting::on_frontend_close();
    controller->onWorldQuit();
    engine->getPaths()->setWorldFolder(fs::path());
}

void LevelScreen::updateHotkeys() {
    auto& settings = engine->getSettings();
    if (Events::jpressed(keycode::O)) {
        settings.graphics.frustumCulling = !settings.graphics.frustumCulling;
    }
    if (Events::jpressed(keycode::F1)) {
        hudVisible = !hudVisible;
    }
    if (Events::jpressed(keycode::F3)) {
        controller->getPlayer()->debug = !controller->getPlayer()->debug;
    }
    if (Events::jpressed(keycode::F5)) {
        controller->getLevel()->chunks->saveAndClear();
    }
}

void LevelScreen::update(float delta) {
    gui::GUI* gui = engine->getGUI();
    
    bool inputLocked = hud->isPause() || 
                       hud->isInventoryOpen() || 
                       gui->isFocusCaught();
    if (!gui->isFocusCaught()) {
        updateHotkeys();
    }

    auto player = controller->getPlayer();
    auto camera = player->camera;
    audio::set_listener(
        camera->position-camera->dir, 
        player->hitbox->velocity,
        camera->dir, 
        camera->up
    );

    // TODO: subscribe for setting change
    EngineSettings& settings = engine->getSettings();
    controller->getPlayer()->camera->setFov(glm::radians(settings.camera.fov));
    if (settings.graphics.backlight != backlight) {
        controller->getLevel()->chunks->saveAndClear();
        backlight = settings.graphics.backlight;
    }

    if (!hud->isPause()) {
        controller->getLevel()->world->updateTimers(delta);
        animator->update(delta);
    }
    controller->update(delta, !inputLocked, hud->isPause());
    hud->update(hudVisible);
}

void LevelScreen::draw(float delta) {
    auto camera = controller->getPlayer()->currentCamera;

    Viewport viewport(Window::width, Window::height);
    GfxContext ctx(nullptr, viewport, batch.get());

    worldRenderer->draw(ctx, camera.get(), hudVisible);

    if (hudVisible) {
        hud->draw(ctx);
    }
}

void LevelScreen::onEngineShutdown() {
    controller->saveWorld();
}

LevelController* LevelScreen::getLevelController() const {
    return controller.get();
}

const unsigned int PRIMITIVE_AABB = 0;
const unsigned int PRIMITIVE_TETRAGON = 1;
const unsigned int PRIMITIVE_HITBOX = 2;

static std::string getTexName(const std::string& fullName) {
	return fullName.substr(fullName.find(':') + 1);
};

static Atlas* getAtlas(Assets* assets, const std::string& fullName) {
	return assets->getAtlas(fullName.substr(0, fullName.find(':')));
};

static std::string getDefName(WorkShopScreen::DefType type) {
	switch (type) {
		case WorkShopScreen::DefType::BLOCK: return "block";
		case WorkShopScreen::DefType::ITEM: return "item";
		case WorkShopScreen::DefType::BOTH: return "[both]";
	}
	return "";
}

static std::string getDefFolder(WorkShopScreen::DefType type) {
	switch (type) {
		case WorkShopScreen::DefType::BLOCK: return ContentPack::BLOCKS_FOLDER.string();
		case WorkShopScreen::DefType::ITEM: return ContentPack::ITEMS_FOLDER.string();
	}
	return "";
}

WorkShopScreen::WorkShopScreen(Engine* engine, const ContentPack& pack) :
	Screen(engine),
	currentPack(pack),
	gui(engine->getGUI()),
	assets(engine->getAssets())
{
	gui->getMenu()->reset();

	initialize();

	uicamera.reset(new Camera(glm::vec3(), Window::height));
	uicamera->perspective = false;
	uicamera->flipped = true;

	auto panel = std::make_shared<gui::Panel>(glm::vec2(250));
	panels.emplace(0, panel);
	panel->setPos(glm::vec2(2.f));
	panel->add(std::make_shared<gui::Button>(L"Info", glm::vec4(10.f), [this, panel](gui::GUI*) {
		createInfoPanel();
	}));
	panel->add(std::make_shared<gui::Button>(L"Blocks", glm::vec4(10.0f), [this, panel](gui::GUI*) {
		createContentList(DefType::BLOCK, 1);
	}));
	panel->add(std::make_shared<gui::Button>(L"Textures", glm::vec4(10.0f), [this, panel](gui::GUI*) {
		createTextureList(50.f, 1);
	}));
	panel->add(std::make_shared<gui::Button>(L"Items", glm::vec4(10.0f), [this, panel](gui::GUI*) {
		createContentList(DefType::ITEM, 1);
	}));
	panel->add(std::make_shared<gui::Button>(L"Back", glm::vec4(10.0f), [engine](gui::GUI*) {
		engine->setScreen(std::make_shared<MenuScreen>(engine));
		engine->getGUI()->getMenu()->setPage("workshop");
	}));

	setNodeColor<gui::Button>(panel);
	gui::Button* button = static_cast<gui::Button*>(panel->getNodes().front().get());
	button->mouseRelease(gui, (int)button->calcPos().x, (int)button->calcPos().y);

	gui->add(panel);
}

WorkShopScreen::~WorkShopScreen() {
	for (const auto& elem : panels) {
		gui->remove(elem.second);
	}
}

void WorkShopScreen::update(float delta) {
	if (Events::jpressed(keycode::ESCAPE)) {
		if (panels.size() < 2) {
			Engine* e = engine;
			e->setScreen(std::make_shared<MenuScreen>(e));
			e->getGUI()->getMenu()->setPage("workshop");
		}
		else removePanel(panels.rbegin()->first);
		return;
	}
	preview->update(delta);

	for (const auto& elem : panels) {
		elem.second->act(delta);
		elem.second->setSize(glm::vec2(elem.second->getSize().x, Window::height - 4.f));
	}
}

void WorkShopScreen::draw(float delta) {
	Window::clear();
	Window::setBgColor(glm::vec3(0.2f));

	preview->draw();

	uicamera->setFov(Window::height);
	Shader* uishader = engine->getAssets()->getShader("ui");
	uishader->use();
	uishader->uniformMatrix("u_projview", uicamera->getProjView());

	uint width = Window::width;
	uint height = Window::height;

	batch->begin();
	batch->texture(engine->getAssets()->getTexture("gui/menubg"));
	batch->rect(0, 0,
		width, height, 0, 0, 0,
		UVRegion(0, 0, width / 64, height / 64),
		false, false, glm::vec4(1.0f));
	batch->flush();
}

void WorkShopScreen::initialize() {
	auto& packs = engine->getContentPacks();
	packs.clear();
	packs.emplace_back(currentPack);
	std::vector<ContentPack> scanned;
	ContentPack::scan(engine->getPaths(), scanned);
	for (const auto& elem : scanned) {
		if (std::find(currentPack.dependencies.begin(), currentPack.dependencies.end(), elem.id) != currentPack.dependencies.end())
			packs.emplace_back(elem);
	}
	engine->loadContent();
	content = engine->getContent();
	indices = content->getIndices();
	cache.reset(new ContentGfxCache(content, assets));
	previewAtlas = BlocksPreview::build(cache.get(), assets, content).release();
	itemsAtlas = assets->getAtlas("items");
	blocksAtlas = assets->getAtlas("blocks");
	preview.reset(new Preview(engine, cache.get()));

	if (!fs::is_regular_file(currentPack.getContentFile()))
		return;
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
}

void WorkShopScreen::removePanel(unsigned int column) {
	if (panels.find(column) == panels.end()) return;
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

	if (posX > 0) {
		panel->setPos(glm::vec2(posX, 2.f));
	}
	else {
		float x = 2.f;
		for (const auto& elem : panels) {
			elem.second->setPos(glm::vec2(x, 2.f));
			x += elem.second->getSize().x;
		}
	}
	gui->add(panel);
}

std::shared_ptr<gui::TextBox> WorkShopScreen::createTextBox(std::shared_ptr<gui::Panel> panel, std::string& string, const std::wstring& placeholder) {
	auto textBox = std::make_shared<gui::TextBox>(placeholder);
	textBox->setText(util::str2wstr_utf8(string));
	textBox->setTextConsumer([&string, textBox](std::wstring text) {
		string = util::wstr2str_utf8(textBox->getInput());
	});
	textBox->setTextSupplier([&string]() {
		return util::str2wstr_utf8(string);
	});
	panel->add(textBox);
	return textBox;
}

template<typename T>
std::shared_ptr<gui::TextBox> WorkShopScreen::createNumTextBox(T& value, const std::wstring& placeholder, T min, T max,
	const std::function<void(T)>& callback) {
	auto textBox = std::make_shared<gui::TextBox>(placeholder);
	textBox->setText(std::to_wstring(value));
	textBox->setTextConsumer([&value, textBox, min, max, callback](std::wstring text) {
		const std::wstring& input = textBox->getInput();
		if (input.empty()) {
			value = 0;
			callback(value);
			return;
		}
		try {
			T result = static_cast<T>(stof(input));
			if (result >= min && result <= max) {
				value = result;
				callback(value);
			}
		} catch (const std::exception&) {}
	});
	textBox->setTextSupplier([&value, min]() {
		if (value != 0) return util::to_wstring(value, (std::is_floating_point<T>::value ? 2 : 0));
		return std::wstring(L"");
	});
	return textBox;
}

std::shared_ptr<gui::FullCheckBox> WorkShopScreen::createFullCheckBox(std::shared_ptr<gui::Panel> panel, const std::wstring& string, bool& isChecked) {
	auto checkbox = std::make_shared<gui::FullCheckBox>(string, glm::vec2(200, 24));
	checkbox->setSupplier([&isChecked]() {
		return isChecked;
	});
	checkbox->setConsumer([&isChecked](bool checked) {
		isChecked = checked;
	});
	panel->add(checkbox);
	return checkbox;
}

std::shared_ptr<gui::RichButton> WorkShopScreen::createTextureButton(const std::string& texture, Atlas* atlas, glm::vec2 size, const wchar_t* side) {
	auto button = std::make_shared<gui::RichButton>(size);
	button->setColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.95f));
	button->setHoverColor(glm::vec4(0.05f, 0.1f, 0.15f, 0.75f));
	auto image = std::make_shared<gui::Image>(atlas->getTexture(), glm::vec2(0.f));
	formatTextureImage(image.get(), atlas, size.y, texture);
	button->add(image);
	auto label = std::make_shared<gui::Label>(util::str2wstr_utf8(texture));
	button->add(label);
	if (side == nullptr) {
		label->setPos(glm::vec2(size.y + 10.f, size.y / 2.f - label->getSize().y / 2.f));
	}
	else {
		label->setPos(glm::vec2(size.y + 10.f, size.y / 2.f));
		label = std::make_shared<gui::Label>(side);
		label->setPos(glm::vec2(size.y + 10.f, size.y / 2.f - label->getSize().y));
		button->add(label);
	}

	return button;
}

std::shared_ptr<gui::Panel> WorkShopScreen::createTexturesPanel(glm::vec2 size, std::string* textures, BlockModel model) {
	auto texturesPanel = std::make_shared<gui::Panel>(glm::vec2(size.x));
	texturesPanel->setColor(glm::vec4(0.f));

	const wchar_t* faces[] = { L"East:", L"West:", L"Bottom:", L"Top:", L"South:", L"North:" };

	size_t buttonsNum = 0;

	switch (model) {
		case BlockModel::block:
		case BlockModel::aabb: buttonsNum = 6;
			break;
		case BlockModel::custom:
		case BlockModel::xsprite: buttonsNum = 1;
			break;
		default:
			break;
	}
	Atlas* atlas = blocksAtlas;
	std::string prefix;
	for (size_t i = 0; i < buttonsNum; i++) {
		std::string texName = textures[i];
		DefType type = DefType::BLOCK;
		if (model == BlockModel::custom) {
			type = DefType::BOTH;
			texName = getTexName(currentBlockIco);
			atlas = getAtlas(assets, currentBlockIco);
		}
		auto button = createTextureButton(texName, atlas, glm::vec2(texturesPanel->getSize().x, size.y), (buttonsNum == 6 ? faces[i] : nullptr));
		button->listenAction([this, button, type, model, textures, size, i](gui::GUI*) {
			createTextureList(35.f, 5, type, button->calcPos().x + button->getSize().x, true,
			[this, button, model, textures, size, i, gui = engine->getGUI()](const std::string& texName) {
				if (model == BlockModel::custom) currentBlockIco = texName;
				else textures[i] = getTexName(texName);
				removePanel(5);
				button->setColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.95f));
				button->setHoverColor(glm::vec4(0.05f, 0.1f, 0.15f, 0.75f));
				auto& nodes = button->getNodes();
				formatTextureImage(static_cast<gui::Image*>(nodes[0].get()), getAtlas(assets, texName), size.y, getTexName(texName));
				static_cast<gui::Label*>(nodes[1].get())->setText(util::str2wstr_utf8(getTexName(texName)));
				preview->updateCache();
				});
			});
		texturesPanel->add(removeList.emplace_back(button));
		setNodeColor<gui::RichButton>(texturesPanel);
	}

	return texturesPanel;
}

std::shared_ptr<gui::Panel> WorkShopScreen::createTexturesPanel(glm::vec2 size, std::string& texture, item_icon_type type) {
	auto texturePanel = std::make_shared<gui::Panel>(glm::vec2(size.x));
	texturePanel->setColor(glm::vec4(0.f));

	Atlas* atlas = previewAtlas;
	std::string texName = texture;
	if (type == item_icon_type::sprite) {
		atlas = getAtlas(assets, texture);
		texName = getTexName(texture);
	}
	else if (type == item_icon_type::none) {
		return texturePanel;
	}

	auto button = createTextureButton(texName, atlas, glm::vec2(texturePanel->getSize().x, size.y));
	button->listenAction([this, button, &texture, size, type](gui::GUI*) {
		auto& nodes = button->getNodes();
		if (type == item_icon_type::sprite) {
			createTextureList(35.f, 5, DefType::BOTH, button->calcPos().x + button->getSize().x, true,
				[this, nodes, size, &texture](const std::string& texName) {
					texture = texName;
					removePanel(5);
					formatTextureImage(static_cast<gui::Image*>(nodes[0].get()), getAtlas(assets, texName), size.y, getTexName(texName));
					static_cast<gui::Label*>(nodes[1].get())->setText(util::str2wstr_utf8(getTexName(texName)));
				});
		}
		else {
			createContentList(DefType::BLOCK, 3, true, [this, nodes, size, &texture](std::string texName) {
				texture = texName;
				removePanel(3);
				formatTextureImage(static_cast<gui::Image*>(nodes[0].get()), previewAtlas, size.y, texName);
				static_cast<gui::Label*>(nodes[1].get())->setText(util::str2wstr_utf8(texName));
			});
		}
		button->setColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.95f));
		button->setHoverColor(glm::vec4(0.05f, 0.1f, 0.15f, 0.75f));
	});
	texturePanel->add(removeList.emplace_back(button));

	return texturePanel;
}

std::shared_ptr<gui::UINode> WorkShopScreen::createVectorPanel(glm::vec3& vec, float min, float max, float width, unsigned int inputType, const std::function<void()>& callback) {
	if (inputType == 0) {
		auto panel = std::make_shared<gui::Panel>(glm::vec2(width));
		panel->setColor(glm::vec4(0.f));
		auto label = std::make_shared<gui::Label>(L"X:" + util::to_wstring(vec.x, 2) + L" Y:" + util::to_wstring(vec.y, 2) + L" Z:" + util::to_wstring(vec.z, 2));
		panel->add(label);

		for (size_t i = 0; i < 3; i++) {
			auto slider = std::make_shared<gui::TrackBar>(min, max, vec[i], 0.01f, 5.f);
			slider->setConsumer([&vec, i, callback, label](double value) {
				vec[i] = static_cast<float>(value);
				label->setText(L"X:" + util::to_wstring(vec.x, 2) + L" Y:" + util::to_wstring(vec.y, 2) + L" Z:" + util::to_wstring(vec.z, 2));
				callback();
				});
			panel->add(slider);
		}
		return panel;
	}
	auto container = std::make_shared<gui::Container>(glm::vec2(0));
	const wchar_t* coords[] = { L"X", L"Y", L"Z" };

	for (glm::vec3::length_type i = 0; i < 3; i++) {
		container->add(createNumTextBox(vec[i], coords[i], min, max, std::function<void(float)>([this, callback](float num) { callback(); })));
	}
	float size = width / 3;
	float height = 0.f;
	size_t i = 0;
	for (auto& elem : container->getNodes()) {
		elem->setSize(glm::vec2(size - elem->getMargin().x - 4, elem->getSize().y));
		elem->setPos(glm::vec2(size * i++, 0.f));
		height = elem->getSize().y;
	}
	container->setSize(glm::vec2(width, height));
	container->setScrollable(false);

	return container;
}

void WorkShopScreen::createEmissionPanel(std::shared_ptr<gui::Panel> panel, uint8_t* emission) {
	panel->add(std::make_shared<gui::Label>("Emission (0 - 15)"));
	wchar_t* colors[] = { L"Red", L"Green", L"Blue" };
	for (size_t i = 0; i < 3; i++) {
		panel->add(createNumTextBox<uint8_t>(emission[i], colors[i], 0, 15));
	}
}

void WorkShopScreen::createContentList(DefType type, unsigned int column, bool showAll, const std::function<void(const std::string&)>& callback) {
	createPanel([this, type, showAll, callback]() {
		auto getDefName = [](DefType type)->std::wstring {
			if (type == DefType::BLOCK) return L"block";
			return L"item";
			};
		auto panel = std::make_shared<gui::Panel>(glm::vec2(200));
		panel->setScrollable(true);
		panel->add(std::make_shared<gui::Button>(L"Create " + getDefName(type), glm::vec4(10.f), [this, type](gui::GUI*) {
			createDefActionPanel(DefAction::CREATE_NEW, type);
		}));

		auto createtList = [this, panel, type, showAll, callback](std::string searchName) {
			Atlas* contentAtlas = (type == DefType::BLOCK ? previewAtlas : itemsAtlas);
			std::vector<std::pair<std::string, ItemDef*>> sorted;
			for (size_t i = !showAll; i < indices->countItemDefs(); i++) {
				std::string fullName, actualName;
				ItemDef* item = indices->getItemDef(i);
				if (type == DefType::ITEM) {
					if (item->generated) continue;
					fullName = item->name;
				}
				else if (type == DefType::BLOCK) {
					if (!item->generated && i != 0) continue;
					fullName = item->placingBlock;
				}
				if (!showAll && fullName.find(currentPack.id) == std::string::npos) continue;
				actualName = fullName.substr(fullName.find(':') + 1);
				if (!searchName.empty() && actualName.find(searchName) == std::string::npos) continue;
				sorted.emplace_back(actualName, item);
			}
			std::sort(sorted.begin(), sorted.end(), [](decltype(sorted[0]) a, decltype(sorted[0]) b) {
				return a.second->placingBlock.compare(b.second->placingBlock) < 0;
			});

			float width = panel->getSize().x;
			for (const auto& elem : sorted) {
				ItemDef* item = elem.second;
				UVRegion uv(0.f, 0.f, 0.f, 0.f);
				if (elem.second->iconType == item_icon_type::block) {
					contentAtlas = previewAtlas;
					uv = contentAtlas->get(item->icon);
				}
				else if (elem.second->iconType == item_icon_type::sprite) {
					contentAtlas = assets->getAtlas(item->icon.substr(0, item->icon.find(':')));
					uv = contentAtlas->get(item->icon.substr(item->icon.find(':') + 1));
				}
				auto label = std::make_shared<gui::Label>((showAll ? elem.second->placingBlock : elem.first));
				auto image = std::make_shared<gui::Image>(contentAtlas->getTexture(), glm::vec2(32, 32));
				auto button = std::make_shared<gui::RichButton>(glm::vec2(width, label->getSize().y * 2));
				button->setColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.95f));
				button->setHoverColor(glm::vec4(0.05f, 0.1f, 0.15f, 0.75f));
				button->add(image, glm::vec2(0.f));
				button->add(label, glm::vec2(image->getSize().x + 8.f, button->getSize().y / 2.f - label->getSize().y / 2.f));
				if (type == DefType::BLOCK) {
					image->setUVRegion(uv);
					button->listenAction([this, elem, panel, callback](gui::GUI*) {
						if (callback) callback(elem.second->placingBlock);
						else {
							createBlockEditor(elem.first);
							preview->setBlock(content->findBlock(elem.second->placingBlock));
						}
					});
				}
				else if (type == DefType::ITEM) {
					image->setUVRegion(uv);
					button->listenAction([this, elem, panel] (gui::GUI*) {
						createItemEditor(elem.first);
					});
				}
				panel->add(removeList.emplace_back(button));
			}
		};
		auto textBox = std::make_shared<gui::TextBox>(L"Search");
		textBox->setTextValidator([=](const std::wstring& text) {
			textBox->setText(textBox->getInput());
			clearRemoveList(panel);
			createtList(util::wstr2str_utf8(textBox->getInput()));
			setNodeColor<gui::RichButton>(panel);
			return true;
		});

		panel->add(textBox);

		createtList(util::wstr2str_utf8(textBox->getInput()));

		setNodeColor<gui::RichButton>(panel);

		return panel;
	}, column);
}

void WorkShopScreen::createInfoPanel() {
	createPanel([this]() {
		std::string icon = currentPack.id + "icon";
		if (assets->getTexture(icon) == nullptr) {
			auto iconfile = currentPack.folder / fs::path("icon.png");
			if (fs::is_regular_file(iconfile)) {
				assets->store(png::load_texture(iconfile.string()), icon);
			}
			else {
				icon = "gui/no_icon";
			}
		}

		auto panel = std::make_shared<gui::Panel>(glm::vec2(400));
		auto iconButton = std::make_shared<gui::RichButton>(glm::vec2(64, 64));
		auto iconImage = std::make_shared<gui::Image>(icon, glm::vec2(64));
		iconButton->add(iconImage);
		auto button = std::make_shared<gui::Button>(L"Change icon", glm::vec4(10.f), gui::onaction());
		button->setPos(glm::vec2(iconImage->getSize().x + 10.f, iconImage->getSize().y / 2.f - button->getSize().y / 2.f));
		iconButton->add(button);
		panel->add(iconButton);
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

void WorkShopScreen::createTextureList(float icoSize, unsigned int column, DefType type, float posX, bool showAll,
	const std::function<void(const std::string&)>& callback)
{
	createPanel([this, showAll, callback, posX, type]() {
		auto panel = std::make_shared<gui::Panel>(glm::vec2(200));
		panel->setScrollable(true);

		auto createList = [this, panel, showAll, callback, posX, type](std::string searchName) {
			auto& packs = engine->getContentPacks();
			std::unordered_map<std::string, fs::path> paths;
			paths.emplace(currentPack.title, currentPack.folder);
			if (showAll) {
				for (const auto& elem : packs) {
					paths[elem.title] = elem.folder;
				}
				paths.emplace("Default", engine->getPaths()->getResources());
			}

			for (const auto& elem : paths) {
				std::unordered_map<fs::path, DefType> defPaths;
				if (type == DefType::BLOCK || type == DefType::BOTH) defPaths.emplace(elem.second / TEXTURES_FOLDER / ContentPack::BLOCKS_FOLDER, DefType::BLOCK);
				if (type == DefType::ITEM || type == DefType::BOTH) defPaths.emplace(elem.second / TEXTURES_FOLDER / ContentPack::ITEMS_FOLDER, DefType::ITEM);
				if (showAll) panel->add(removeList.emplace_back(std::make_shared<gui::Label>(elem.first)));
				for (const auto& defPath : defPaths) {
					Atlas* atlas = assets->getAtlas(getDefFolder(defPath.second));
					if (!fs::exists(defPath.first)) continue;
					for (const auto& entry : fs::directory_iterator(defPath.first)) {
						std::string file = entry.path().stem().string();
						if (!searchName.empty()) {
							if (file.find(searchName) == std::string::npos) continue;
						}
						if (!atlas->has(file)) continue;
						auto button = createTextureButton(file, atlas, glm::vec2(panel->getSize().x, 50.f));
						button->listenAction([this, panel, file, defPath, callback, posX](gui::GUI*) {
							if (callback) callback(getDefFolder(defPath.second) + ':' + file);
							else createTextureInfoPanel(getDefFolder(defPath.second) + ':' + file, defPath.second);
						});
						panel->add(removeList.emplace_back(button));
					}
				}
			}
		};
		if (!showAll) {
			panel->add(std::make_shared<gui::Button>(L"Import", glm::vec4(10.f), [this](gui::GUI*) {
				createImportPanel();
			}));
		}
		auto textBox = std::make_shared<gui::TextBox>(L"Search");
		textBox->setTextValidator([=](const std::wstring& text) {
			textBox->setText(textBox->getInput());
			clearRemoveList(panel);
			createList(util::wstr2str_utf8(textBox->getInput()));
			setNodeColor<gui::RichButton>(panel);
			return true;
		});

		panel->add(textBox);

		createList(util::wstr2str_utf8(textBox->getInput()));

		setNodeColor<gui::RichButton>(panel);

		return panel;
	}, column, posX);
}

void WorkShopScreen::createDefActionPanel(DefAction action, DefType type, const std::string& name) {
	createPanel([this, action, type, name]() {
		auto panel = std::make_shared<gui::Panel>(glm::vec2(200));

		const wchar_t* buttonItem[] = { L"Item name", L"Rename item", L"Delete item" };
		const wchar_t* buttonBlock[] = { L"Block name", L"Rename block", L"Delete block" };
		const wchar_t* buttonAct[] = { L"Create", L"Rename", L"Delete" };
		const wchar_t** buttonType = (type == DefType::BLOCK ? buttonBlock : buttonItem);

		panel->add(std::make_shared<gui::Label>(buttonType[static_cast<int>(action)]));
		std::shared_ptr<gui::TextBox> nameInput;
		if (action == DefAction::DELETE) {
			panel->add(std::make_shared<gui::Label>(name+ "?"));
		}
		else {
			nameInput = std::make_shared<gui::TextBox>((type == DefType::BLOCK ? L"example_block" : L"example_item"));
			nameInput->setTextValidator([this, nameInput](const std::wstring& text) {
				std::string input(util::wstr2str_utf8(nameInput->getInput()));
				return blocksList.find(input) == blocksList.end() && util::is_valid_filename(nameInput->getInput()) && !input.empty();
			});
			panel->add(nameInput);
		}
		panel->add(std::make_shared<gui::Button>(buttonAct[static_cast<int>(action)], glm::vec4(10.f), [this, nameInput, action, name, type](gui::GUI*) {
			if (nameInput && !nameInput->validate()) return;
			fs::path path(currentPack.folder / (type == DefType::BLOCK ? ContentPack::BLOCKS_FOLDER : ContentPack::ITEMS_FOLDER));
			if (!fs::is_directory(path)) fs::create_directory(path);
			if (action == DefAction::CREATE_NEW) {
				dynamic::Map map;
				files::write_json(path / (util::wstr2str_utf8(nameInput->getInput()) + ".json"), &map);
			}
			else if (action == DefAction::RENAME) {
				fs::rename(path / (name + ".json"), path / (util::wstr2str_utf8(nameInput->getInput()) + ".json"));
			}
			else if (action == DefAction::DELETE) {
				if (type == DefType::BLOCK) {
					fs::path blockIcoFilePath(currentPack.folder / ContentPack::ITEMS_FOLDER / fs::path(name + BLOCK_ITEM_SUFFIX + ".json"));
					if (fs::is_regular_file(blockIcoFilePath)) fs::remove(blockIcoFilePath);
				}
				fs::remove(path / (name + ".json"));
			}
			initialize();
			createContentList(type, 1);
		}));

		return panel;
	}, 2);
}

void WorkShopScreen::createTextureInfoPanel(const std::string& texName, DefType type) {
	createPanel([this, texName, type]() {
		auto panel = std::make_shared<gui::Panel>(glm::vec2(350));

		auto imageContainer = std::make_shared<gui::Container>(glm::vec2(panel->getSize().x));
		Atlas* atlas = getAtlas(assets, texName);
		Texture* tex = atlas->getTexture();
		auto image = std::make_shared<gui::Image>(tex, glm::vec2(0.f));
		formatTextureImage(image.get(), atlas, panel->getSize().x, getTexName(texName));
		imageContainer->add(image);
		panel->add(imageContainer);
		const UVRegion& uv = atlas->get(getTexName(texName));
		glm::ivec2 size((uv.u2 - uv.u1) * tex->getWidth(), (uv.v2 - uv.v1) * tex->getHeight());
		panel->add(std::make_shared<gui::Label>(L"Width: " + std::to_wstring(size.x)));
		panel->add(std::make_shared<gui::Label>(L"Height: " + std::to_wstring(size.y)));
		panel->add(std::make_shared<gui::Label>(L"Texture type: " + util::str2wstr_utf8(getDefName(type))));
		panel->add(std::make_shared<gui::Button>(L"Delete", glm::vec4(10.f), [this, texName, type](gui::GUI*) {
			fs::remove(currentPack.folder / TEXTURES_FOLDER / getDefFolder(type) / std::string(getTexName(texName) + ".png"));
			initialize();
			createTextureList(50.f, 1);
		}));
		return panel;
	}, 2);
}

void WorkShopScreen::createImportPanel(DefType type) {
	createPanel([this, type]() {
		auto panel = std::make_shared<gui::Panel>(glm::vec2(200));

		panel->add(std::make_shared<gui::Label>(L"Import texture"));
		panel->add(std::make_shared<gui::Button>(L"Import as: " + util::str2wstr_utf8(getDefName(type)), glm::vec4(10.f), [this, type](gui::GUI*) {
			switch (type) {
				case DefType::BLOCK: createImportPanel(DefType::ITEM); break;
				case DefType::ITEM: createImportPanel(DefType::BLOCK); break;
			}
		}));
		panel->add(std::make_shared<gui::Button>(L"Select files", glm::vec4(10.f), [this, type](gui::GUI*) {
			std::vector<fs::path> files;
#ifdef _WIN32
			if (FAILED(MultiselectInvoke(files))) return;
#endif // _WIN32
			fs::path path(currentPack.folder / TEXTURES_FOLDER / (type == DefType::BLOCK ? ContentPack::BLOCKS_FOLDER : ContentPack::ITEMS_FOLDER));
			if (!fs::is_directory(path)) fs::create_directories(path);
			for (const auto& elem : files) {
				fs::copy(elem, path);
			}
			initialize();
			createTextureList(50.f, 1);
		}));

		return panel;
	}, 2);
}

void WorkShopScreen::createBlockEditor(const std::string& blockName) {
	currentBlockIco.clear();
	createPanel([this, blockName]() {
		Block* block = content->findBlock(currentPack.id + ":" + blockName);
		ItemDef* item = content->findItem(block->name + BLOCK_ITEM_SUFFIX);
		currentBlockIco = (item->icon == block->name ? "blocks:notfound" : item->icon);

		auto panel = std::make_shared<gui::Panel>(glm::vec2(200));

		panel->add(std::make_shared<gui::Label>(blockName));
		createFullCheckBox(panel, L"Light passing", block->lightPassing);
		createFullCheckBox(panel, L"Sky light passing", block->skyLightPassing);
		createFullCheckBox(panel, L"Obstacle", block->obstacle);
		createFullCheckBox(panel, L"Selectable", block->selectable);
		createFullCheckBox(panel, L"Replaceable", block->replaceable);
		createFullCheckBox(panel, L"Breakable", block->breakable);
		createFullCheckBox(panel, L"Grounded", block->grounded);
		createFullCheckBox(panel, L"Hidden", block->hidden);

		bool singleTexture = std::all_of(std::cbegin(block->textureFaces), std::cend(block->textureFaces), [&r = block->textureFaces[0]](const std::string& value) {return value == r; });

		auto texturePanel = createTexturesPanel(glm::vec2(panel->getSize().x, 35.f), block->textureFaces, block->model);
		panel->add(texturePanel);

		char* models[] = { "none", "block", "X", "aabb", "custom" };
		auto button = std::make_shared<gui::Button>(L"Model: " + util::str2wstr_utf8(models[static_cast<size_t>(block->model)]), glm::vec4(10.f), gui::onaction());
		auto processModelChange = [this, panel, texturePanel](Block* block) {
			clearRemoveList(texturePanel);
			auto temp = createTexturesPanel(glm::vec2(panel->getSize().x, 35.f), block->textureFaces, block->model);
			for (auto& elem : temp->getNodes()) {
				texturePanel->add(elem);
			}
			switch (block->model) {
				case BlockModel::custom: createCustomModelEditor(block, 0, PRIMITIVE_AABB); break;
				default: createCustomModelEditor(block, 0, PRIMITIVE_HITBOX); break;
			}
			texturePanel->cropToContent();
			preview->updateMesh();
		};
		button->listenAction([block, button, models, processModelChange](gui::GUI*) {
			size_t index = static_cast<size_t>(block->model) + 1;
			if (index >= std::size(models)) index = 0;
			button->setText(L"Model: " + util::str2wstr_utf8(models[index]));
			block->model = static_cast<BlockModel>(index);
			processModelChange(block);
		});
		processModelChange(block);

		panel->add(button);

		auto getRotationName = [](const Block* block) {
			if (!block->rotatable) return "none";
			return block->rotations.name.data();
		};

		button = std::make_shared<gui::Button>(L"Rotation: " + util::str2wstr_utf8(getRotationName(block)), glm::vec4(10.f), gui::onaction());
		button->listenAction([block, getRotationName, button](gui::GUI*) {
			std::string rot = getRotationName(block);
			block->rotatable = true;
			if (rot == "none") {
				block->rotations = BlockRotProfile::PANE;
			}
			else if (rot == "pane") {
				block->rotations = BlockRotProfile::PIPE;
			}
			else if (rot == "pipe") {
				block->rotatable = false;
			}
			button->setText(L"Rotation: " + util::str2wstr_utf8(getRotationName(block)));
		});
		panel->add(button);

		panel->add(std::make_shared<gui::Label>("Draw group"));
		panel->add(createNumTextBox<ubyte>(block->drawGroup, L"0", 0, 255));
		createEmissionPanel(panel, block->emission);

		panel->add(std::make_shared<gui::Button>(L"Save", glm::vec4(10.f), [this, block, blockName](gui::GUI*) {
			saveBlock(block, blockName);
		}));
		panel->add(std::make_shared<gui::Button>(L"Rename", glm::vec4(10.f), [this, blockName](gui::GUI*) {
			createDefActionPanel(DefAction::RENAME, DefType::BLOCK, blockName);
		}));
		panel->add(std::make_shared<gui::Button>(L"Delete", glm::vec4(10.f), [this, blockName](gui::GUI*) {
			createDefActionPanel(DefAction::DELETE, DefType::BLOCK, blockName);
		}));
		
		return panel;
	}, 2);
}

void WorkShopScreen::createCustomModelEditor(Block* block, size_t index, unsigned int primitiveType) {
	createPanel([this, block, index, primitiveType]() mutable {
		auto panel = std::make_shared<gui::Panel>(glm::vec2(200));
		createPreview(4, primitiveType);

		std::vector<AABB>& aabbArr = (primitiveType == PRIMITIVE_AABB ? block->modelBoxes : block->hitboxes);
		const std::wstring primitives[] = { L"AABB", L"Tetragon", L"Hitbox" };
		const std::wstring editorModes[] = { L"Custom model", L"Custom model", L"Hitbox" };
		const glm::vec3 tetragonTemplate[] = { glm::vec3(0.f, 0.f, 0.5f), glm::vec3(1.f, 0.f, 0.5f), glm::vec3(1.f, 1.f, 0.5f), glm::vec3(0.f, 1.f, 0.5f) };
		if (block->model == BlockModel::custom) {
			panel->add(std::make_shared<gui::Button>(L"Mode: " + editorModes[primitiveType], glm::vec4(10.f), [this, block, primitiveType](gui::GUI*) {
				if (primitiveType == PRIMITIVE_HITBOX) createCustomModelEditor(block, 0, PRIMITIVE_AABB);
				else createCustomModelEditor(block, 0, PRIMITIVE_HITBOX);
			}));
		}
		if (primitiveType != PRIMITIVE_HITBOX) {
			panel->add(std::make_shared<gui::Button>(L"Primitive: " + primitives[primitiveType], glm::vec4(10.f), [this, block, primitiveType](gui::GUI*) {
				if (primitiveType == PRIMITIVE_AABB) createCustomModelEditor(block, 0, PRIMITIVE_TETRAGON);
				else if (primitiveType == PRIMITIVE_TETRAGON) createCustomModelEditor(block, 0, PRIMITIVE_AABB);

			}));
		}
		size_t size = (primitiveType == PRIMITIVE_TETRAGON ? block->modelExtraPoints.size() / 4 : aabbArr.size());
		panel->add(std::make_shared<gui::Label>(primitives[primitiveType] + L": " + std::to_wstring(index + 1) + L"/" + std::to_wstring(size)));

		panel->add(std::make_shared<gui::Button>(L"Add new", glm::vec4(10.f), [this, block, primitiveType, &aabbArr, tetragonTemplate](gui::GUI*) {
			size_t index = 0;
			if (primitiveType == PRIMITIVE_TETRAGON) {
				block->modelTextures.emplace_back("notfound");
				for (size_t i = 0; i < 4; i++) {
					block->modelExtraPoints.emplace_back(tetragonTemplate[i]);
				}
				index = block->modelExtraPoints.size() / 4;
			}
			else {
				if (primitiveType == PRIMITIVE_AABB)
					block->modelTextures.insert(block->modelTextures.begin() + aabbArr.size() * 6, 6, "notfound");
				aabbArr.emplace_back(AABB());
				index = aabbArr.size();
			}
			createCustomModelEditor(block, index - 1, primitiveType);
			if (primitiveType != PRIMITIVE_HITBOX) preview->updateCache();
		}));

		if (primitiveType == PRIMITIVE_AABB && block->modelBoxes.empty() || 
			primitiveType == PRIMITIVE_TETRAGON && block->modelExtraPoints.empty() ||
			primitiveType == PRIMITIVE_HITBOX && block->hitboxes.empty()) return panel;
		panel->add(std::make_shared<gui::Button>(L"Copy current", glm::vec4(10.f), [this, block, index, primitiveType, &aabbArr](gui::GUI*) {
			if (primitiveType == PRIMITIVE_TETRAGON) {
				block->modelTextures.emplace_back(*(block->modelTextures.begin() + block->modelBoxes.size() * 6 + index));
				for (size_t i = 0; i < 4; i++) {
					block->modelExtraPoints.emplace_back(*(block->modelExtraPoints.begin() + index * 4 + i));
				}
			}
			else {
				if (primitiveType == PRIMITIVE_AABB) {
					for (size_t i = 0; i < 6; i++) {
						block->modelTextures.emplace(block->modelTextures.begin() + block->modelBoxes.size() * 6 + i, *(block->modelTextures.begin() + index * 6 + i));
					}
				}
				aabbArr.emplace_back(aabbArr[index]);
			}
			createCustomModelEditor(block, (primitiveType == PRIMITIVE_TETRAGON ? block->modelExtraPoints.size() / 4 : aabbArr.size() - 1), primitiveType);
			if (primitiveType != PRIMITIVE_HITBOX) preview->updateCache();
		}));
		panel->add(std::make_shared<gui::Button>(L"Remove current", glm::vec4(10.f), [this, block, index, primitiveType, &aabbArr](gui::GUI*) {
			if (primitiveType == PRIMITIVE_TETRAGON) {
				auto it = block->modelExtraPoints.begin() + index * 4;
				block->modelExtraPoints.erase(it, it + 4);
				block->modelTextures.erase(block->modelTextures.begin() + block->modelBoxes.size() * 6 + index);
				createCustomModelEditor(block, std::min(block->modelExtraPoints.size() / 4 - 1, index), primitiveType);
			}
			else {
				if (primitiveType == PRIMITIVE_AABB) {
					auto it = block->modelTextures.begin() + index * 6;
					block->modelTextures.erase(it, it + 6);
				}
				aabbArr.erase(aabbArr.begin() + index);
				createCustomModelEditor(block, std::min(aabbArr.size() - 1, index), primitiveType);
			}
			if (primitiveType != PRIMITIVE_HITBOX) preview->updateCache();
		}));
		panel->add(std::make_shared<gui::Button>(L"Previous", glm::vec4(10.f), [this, block, index, primitiveType](gui::GUI*) {
			if (index != 0) {
				createCustomModelEditor(block, index - 1, primitiveType);
			}
		}));
		panel->add(std::make_shared<gui::Button>(L"Next", glm::vec4(10.f), [this, block, index, primitiveType, &aabbArr](gui::GUI*) {
			size_t place = (primitiveType == PRIMITIVE_TETRAGON ? index * 4 + 4 : index + 1);
			size_t size = (primitiveType == PRIMITIVE_TETRAGON ? block->modelExtraPoints.size() : aabbArr.size());
			if (place < size) {
				createCustomModelEditor(block, index + 1, primitiveType);
			}
		}));

		auto updateTetragon = [](glm::vec3* p) {
			glm::vec3 p1 = p[0];
			glm::vec3 xw = p[1] - p1;
			glm::vec3 yh = p[3] - p1;
			p[2] = p1 + xw + yh;
		};

		static unsigned int inputMode = 0;
		auto createInputModeButton = [](const std::function<void(unsigned int)>& callback) {
			std::wstring inputModes[] = { L"Slider", L"InputBox" };
			auto button = std::make_shared<gui::Button>(L"Input mode: " + inputModes[inputMode], glm::vec4(10.f), gui::onaction());
			button->listenAction([button, &m = inputMode, inputModes, callback](gui::GUI*) {
				if (++m >= 2) m = 0;
				button->setText(L"Input mode: " + inputModes[m]);
				callback(m);
			});
			return button;
		};

		auto inputPanel = std::make_shared<gui::Panel>(glm::vec2(panel->getSize().x));
		inputPanel->setColor(glm::vec4(0.f));
		panel->add(inputPanel);

		std::function<void(unsigned int)> createInput;
		std::string* textures = nullptr;
		if (primitiveType == PRIMITIVE_TETRAGON) {
			preview->setCurrentTetragon(&block->modelExtraPoints[index * 4]);

			createInput = [this, block, index, inputPanel, panel, updateTetragon](unsigned int type) {
				clearRemoveList(inputPanel);
				for (size_t i = 0; i < 4; i++) {
					if (i == 2) continue;
					glm::vec3* vec = &block->modelExtraPoints[index * 4];
					inputPanel->add(removeList.emplace_back(createVectorPanel(vec[i], 0.f, 1.f, panel->getSize().x, type,
						[this, updateTetragon, vec]() {
						updateTetragon(vec);
						preview->setCurrentTetragon(vec);
						preview->updateMesh();
					})));
				}
			};
			textures = block->modelTextures.data() + block->modelBoxes.size() * 6 + index;
		}
		else {
			AABB& aabb = aabbArr[index];
			preview->setCurrentAABB(aabb, primitiveType);

			createInput = [this, &aabb, inputPanel, panel, primitiveType](unsigned int type) {
				clearRemoveList(inputPanel);
				inputPanel->add(removeList.emplace_back(createVectorPanel(aabb.a, 0.f, 1.f, panel->getSize().x, type, 
					[this, &aabb, primitiveType]() { preview->setCurrentAABB(aabb, primitiveType); preview->updateMesh(); })));
				inputPanel->add(removeList.emplace_back(createVectorPanel(aabb.b, 0.f, 1.f, panel->getSize().x, type, 
					[this, &aabb, primitiveType]() { preview->setCurrentAABB(aabb, primitiveType); preview->updateMesh(); })));
			};
			textures = block->modelTextures.data() + index * 6;
		}
		createInput(inputMode);
		panel->add(createInputModeButton(createInput));
		if (primitiveType != PRIMITIVE_HITBOX)
			panel->add(createTexturesPanel(glm::vec2(panel->getSize().x, 35.f), textures, (primitiveType == PRIMITIVE_AABB ? BlockModel::aabb : BlockModel::xsprite)));

		return panel;
	}, 3);
}

void WorkShopScreen::createItemEditor(const std::string& itemName) {
	createPanel([this, itemName]() {
		ItemDef* item = content->findItem(currentPack.id + ":" + itemName);

		auto panel = std::make_shared<gui::Panel>(glm::vec2(200));
		panel->add(std::make_shared<gui::Label>(itemName));
		panel->add(std::make_shared<gui::Label>(L"Stack size:"));
		panel->add(createNumTextBox<uint32_t>(item->stackSize, L"64", 0, 64));
		createEmissionPanel(panel, item->emission);

		panel->add(std::make_shared<gui::Label>(L"Icon"));
		auto textureIco = createTexturesPanel(glm::vec2(panel->getSize().x, 35.f), item->icon, item->iconType);
		panel->add(textureIco);

		auto processIconChange = [this, panel, item, textureIco](item_icon_type type, const std::string& icon) {
			clearRemoveList(textureIco);
			auto temp = createTexturesPanel(glm::vec2(panel->getSize().x, 35.f), item->icon, item->iconType);
			for (auto& elem : temp->getNodes()) {
				textureIco->add(elem);
			}
		};

		processIconChange(item->iconType, item->icon);
		wchar_t* iconTypes[] = { L"none", L"sprite", L"block" };
		auto button = std::make_shared<gui::Button>(L"Icon type: " + std::wstring(iconTypes[static_cast<unsigned int>(item->iconType)]), glm::vec4(10.f), gui::onaction());
		button->listenAction([this, button, iconTypes, item, textureIco, processIconChange](gui::GUI*) {
			switch (item->iconType) {
				case item_icon_type::block:
					item->iconType = item_icon_type::none;
					item->icon = "core:air";
					break;
				case item_icon_type::none:
					item->iconType = item_icon_type::sprite;
					item->icon = "blocks:notfound";
					break;
				case item_icon_type::sprite:
					item->iconType = item_icon_type::block;
					item->icon = "core:air";
					break;
			}
			removePanels(3);
			button->setText(L"Icon type: " + std::wstring(iconTypes[static_cast<unsigned int>(item->iconType)]));
			processIconChange(item->iconType, item->icon);
			textureIco->cropToContent();
		});
		panel->add(button);

		panel->add(std::make_shared<gui::Label>(L"Placing block"));
		panel->add(createTexturesPanel(glm::vec2(panel->getSize().x, 35.f), item->placingBlock, item_icon_type::block));
		panel->add(std::make_shared<gui::Button>(L"Save", glm::vec4(10.f), [this, itemName, item](gui::GUI*) {
			saveItem(item, itemName);
		}));
		panel->add(std::make_shared<gui::Button>(L"Delete", glm::vec4(10.f), [this, itemName](gui::GUI*) {
			createDefActionPanel(DefAction::DELETE, DefType::ITEM, itemName);
		}));

		return panel;
	}, 2);
}

void WorkShopScreen::createPreview(unsigned int column, unsigned int primitiveType) {
	createPanel([this, primitiveType]() {
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
			preview->setResolution(image->getSize().x, image->getSize().y);
			image->setTexture(preview->getTexture());
		});
		preview->drawBlockHitbox = false;
		createFullCheckBox(panel, L"Draw grid", preview->drawGrid);
		createFullCheckBox(panel, L"Draw block bounds", preview->drawBlockBounds);
		if (primitiveType == PRIMITIVE_HITBOX)
			createFullCheckBox(panel, L"Draw current hitbox", preview->drawBlockHitbox = true);
		else if (primitiveType == PRIMITIVE_AABB)
			createFullCheckBox(panel, L"Highlight current AABB", preview->drawCurrentAABB);
		else if (primitiveType == PRIMITIVE_TETRAGON)
			createFullCheckBox(panel, L"Highlight current Tetragon", preview->drawCurrentTetragon);
		return panel;
	}, column);
}

void WorkShopScreen::saveBlock(Block* block, const std::string& actualName) const {
	Block temp("");
	dynamic::Map root;
	if (block->model != BlockModel::custom) {
		if (std::all_of(std::cbegin(block->textureFaces), std::cend(block->textureFaces), [&r = block->textureFaces[0]](const std::string& value) {return value == r; }) ||
			block->model == BlockModel::xsprite) {
			root.put("texture", block->textureFaces[0]);
		}
		else {
			auto& texarr = root.putList("texture-faces");
			for (size_t i = 0; i < 6; i++) {
				texarr.put(block->textureFaces[i]);
			}
		}
	}

	if (temp.lightPassing != block->lightPassing) root.put("light-passing", block->lightPassing);
	if (temp.skyLightPassing != block->skyLightPassing) root.put("sky-light-passing", block->skyLightPassing);
	if (temp.obstacle != block->obstacle) root.put("obstacle", block->obstacle);
	if (temp.selectable != block->selectable) root.put("selectable", block->selectable);
	if (temp.replaceable != block->replaceable) root.put("replaceable", block->replaceable);
	if (temp.breakable != block->breakable) root.put("breakable", block->breakable);
	if (temp.grounded != block->grounded) root.put("grounded", block->grounded);
	if (temp.drawGroup != block->drawGroup) root.put("draw-group", block->drawGroup);
	const char *models[] = {"none", "block", "X", "aabb", "custom"};
	if (temp.model != block->model) root.put("model", models[static_cast<size_t>(block->model)]);
	if (block->emission[0] || block->emission[1] || block->emission[2]) {
		auto& emissionarr = root.putList("emission");
		emissionarr.multiline = false;
		for (size_t i = 0; i < 3; i++) {
			emissionarr.put(block->emission[i]);
		}
	}
	if (block->rotatable) {
		root.put("rotation", block->rotations.name);
	}
	auto putVec3 = [](dynamic::List& list, const glm::vec3& vector) {
		list.put(vector.x); list.put(vector.y); list.put(vector.z);
	};
	auto putAABB = [putVec3](dynamic::List& list, AABB aabb) {
		aabb.b -= aabb.a;
		putVec3(list, aabb.a); putVec3(list, aabb.b);
	};
	auto& hitboxesArr = root.putList("hitboxes");
	for (const auto& hitbox : block->hitboxes) {
		auto& hitboxArr = hitboxesArr.putList();
		hitboxArr.multiline = false;
		putAABB(hitboxArr, hitbox);
	}

	auto isElementsEqual = [](const std::vector<std::string>& arr, size_t offset, size_t numElements) {
		return std::all_of(std::cbegin(arr) + offset, std::cbegin(arr) + offset + numElements, [&r = arr[offset]](const std::string& value) {return value == r; });
	};
	fs::path blockIcoFilePath(currentPack.folder / ContentPack::ITEMS_FOLDER / fs::path(actualName + BLOCK_ITEM_SUFFIX + ".json"));
	if (block->model == BlockModel::custom) {
		dynamic::Map blockIco;
		blockIco.put("icon-type", "sprite");
		blockIco.put("icon", currentBlockIco);
		files::write_json(blockIcoFilePath, &blockIco);

		dynamic::Map& model = root.putMap("model-primitives");
		size_t offset = 0;
		if (!block->modelBoxes.empty()) {
			dynamic::List& aabbs = model.putList("aabbs");
			for (size_t i = 0; i < block->modelBoxes.size(); i++) {
				dynamic::List& aabb = aabbs.putList();
				aabb.multiline = false;
				putAABB(aabb, block->modelBoxes[i]);
				size_t textures = (isElementsEqual(block->modelTextures, offset, 6) ? 1 : 6);
				for (size_t j = 0; j < textures; j++) {
					aabb.put(block->modelTextures[offset + j]);
				}
				offset += 6;
			}
		}
		if (!block->modelExtraPoints.empty()) {
			dynamic::List& tetragons = model.putList("tetragons");
			for (size_t i = 0; i < block->modelExtraPoints.size(); i += 4) {
				dynamic::List& tetragon = tetragons.putList();
				tetragon.multiline = false;
				putVec3(tetragon, block->modelExtraPoints[i]);
				putVec3(tetragon, block->modelExtraPoints[i + 1] - block->modelExtraPoints[i]);
				putVec3(tetragon, block->modelExtraPoints[i + 3] - block->modelExtraPoints[i]);
				tetragon.put(block->modelTextures[offset++]);
			}
		}
	}
	else if (fs::is_regular_file(blockIcoFilePath)) fs::remove(blockIcoFilePath);

	json::precision = 3;
	fs::path path = currentPack.folder/ContentPack::BLOCKS_FOLDER;
	files::write_json(path/fs::path(actualName + ".json"), &root);
	json::precision = 15;
}

void WorkShopScreen::saveItem(ItemDef* item, const std::string& actualName) const {
	ItemDef temp("");
	dynamic::Map root;
	if (temp.stackSize != item->stackSize) root.put("stack-size", item->stackSize);
	if (item->emission[0] || item->emission[1] || item->emission[2]) {
		auto& emissionarr = root.putList("emission");
		emissionarr.multiline = false;
		for (size_t i = 0; i < 3; i++) {
			emissionarr.put(item->emission[i]);
		}
	}
	std::string iconStr("");
	switch (item->iconType) {
	case item_icon_type::none: iconStr = "none";
		break;
	case item_icon_type::sprite: iconStr = "sprite";
		break;
	case item_icon_type::block: iconStr = "block";
		break;
	}
	if (!iconStr.empty()) root.put("icon-type", iconStr);
	root.put("icon", item->icon);
	root.put("placing-block", item->placingBlock);

	fs::path path = currentPack.folder / ContentPack::ITEMS_FOLDER;
	files::write_json(path / fs::path(actualName + ".json"), &root);
}

void WorkShopScreen::formatTextureImage(gui::Image* image, Atlas* atlas, float height, const std::string& texName) {
	const UVRegion& region = atlas->get(texName);
	glm::vec2 textureSize((region.u2 - region.u1) * atlas->getTexture()->getWidth(), (region.v2 - region.v1) * atlas->getTexture()->getHeight());
	glm::vec2 multiplier(height / textureSize);
	image->setSize(textureSize * std::min(multiplier.x, multiplier.y));
	image->setPos(glm::vec2(height / 2.f - image->getSize().x / 2.f, height / 2.f - image->getSize().y / 2.f));
	image->setTexture(atlas->getTexture());
	image->setUVRegion(region);
}

WorkShopScreen::Preview::Preview(Engine* engine, ContentGfxCache* cache) : engine(engine), cache(cache) {
	auto content = engine->getContent();
	blockRenderer.reset(new BlocksRenderer(8192, content, cache, engine->getSettings()));
	chunk.reset(new Chunk(0, 0));
	world = new World("temp", "", "", 0, engine->getSettings(), content, engine->getContentPacks());
	level.reset(new Level(world, content, engine->getSettings()));
	level->chunksStorage->store(chunk);
	camera.reset(new Camera(glm::vec3(0.f), glm::radians(60.f)));
	memset(chunk->voxels, 0, sizeof(chunk->voxels));
	framebuffer.reset(new Framebuffer(720, 540, true));

	lineBatch.reset(new LineBatch(1024));
}

void WorkShopScreen::Preview::update(float delta) {
	if (Events::pressed(keycode::D)) rotate(100.f * delta, 0.f);
	if (Events::pressed(keycode::A)) rotate(-100.f * delta, 0.f);
	if (Events::pressed(keycode::S)) rotate(0.f, 100.f * delta);
	if (Events::pressed(keycode::W)) rotate(0.f,-100.f * delta);
	if (Events::pressed(keycode::SPACE)) scale(2.f * delta);
	if (Events::pressed(keycode::LEFT_SHIFT)) scale(-2.f * delta);
	if (mouseLocked) {
		rotate(-Events::delta.x / 2.f, Events::delta.y / 2.f);
		if (!Events::clicked(mousecode::BUTTON_1)) mouseLocked = false;
	}
}

void WorkShopScreen::Preview::updateMesh() {
	mesh.reset(blockRenderer->render(chunk.get(), level->chunksStorage.get()));
}

void WorkShopScreen::Preview::updateCache() {
	auto assets = engine->getAssets();
	auto content = engine->getContent();
	cache->refresh(content, assets);
	updateMesh();
}

void WorkShopScreen::Preview::setBlock(Block* block) {
	chunk->voxels[CHUNK_D * CHUNK_W + CHUNK_D + 1].id = block->rt.id;
	bool rotatable = block->rotatable;
	block->rotatable = false;
	updateMesh();
	block->rotatable = rotatable;
}

void WorkShopScreen::Preview::setCurrentAABB(const AABB& aabb, unsigned int primitiveType) {
	AABB& aabb_ = (primitiveType == PRIMITIVE_AABB ? currentAABB : currentHitbox);
	aabb_.a = aabb.a + (aabb.b - aabb.a) / 2.f - 0.5f;
	aabb_.b = aabb.b - aabb.a;
}

void WorkShopScreen::Preview::setCurrentTetragon(const glm::vec3* tetragon) {
	for (size_t i = 0; i < 4; i++) {
		currentTetragon[i] = tetragon[i] - 0.5f;
	}
}

void WorkShopScreen::Preview::setResolution(uint width, uint height) {
	if (framebuffer->getWidth() == width && framebuffer->getHeight() == height) return;
	framebuffer.reset(new Framebuffer(width, height, true));
	camera->aspect = (float)width / height;
}

Texture* WorkShopScreen::Preview::getTexture() {
	return framebuffer->getTexture();
}

void WorkShopScreen::Preview::rotate(float x, float y) {
	previewRotation.x += x;
	previewRotation.y = std::clamp(previewRotation.y + y, -89.f, 89.f);
}

void WorkShopScreen::Preview::scale(float value) {
	viewDistance = std::clamp(viewDistance + value, 1.f, 5.f);
}

void WorkShopScreen::Preview::draw() {
	if (mesh == nullptr) return;
	Window::viewport(0, 0, framebuffer->getWidth(), framebuffer->getHeight());
	framebuffer->bind();
	Window::setBgColor(glm::vec4(0.f));
	Window::clear();
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	Assets* assets = engine->getAssets();
	Shader* lineShader = assets->getShader("lines");
	Shader* shader = assets->getShader("main");
	Texture* texture = assets->getAtlas("blocks")->getTexture();

	camera->rotation = glm::mat4(1.f);
	camera->rotate(glm::radians(previewRotation.y), glm::radians(previewRotation.x), 0);
	camera->position = camera->front * viewDistance;
	camera->dir *= -1.f;
	camera->front *= -1.f;

	if (drawGrid) {
		for (float i = -3.f; i < 3; i++) {
			lineBatch->line(glm::vec3(i + 0.5f, -0.5f, -3.f), glm::vec3(i + 0.5f, -0.5f, 3.f), glm::vec4(0.f, 0.f, 1.f, 1.f));
		}
		for (float i = -3.f; i < 3; i++) {
			lineBatch->line(glm::vec3(-3.f, -0.5f, i + 0.5f), glm::vec3(3.f, -0.5f, i + 0.5f), glm::vec4(1.f, 0.f, 0.f, 1.f));
		}
	}
	if (drawBlockBounds) lineBatch->box(glm::vec3(0.f), glm::vec3(1.f), glm::vec4(1.f));
	if (drawBlockHitbox) lineBatch->box(currentHitbox.a, currentHitbox.b, glm::vec4(1.f, 1.f, 0.f, 1.f));
	if (drawCurrentAABB) lineBatch->box(currentAABB.a, currentAABB.b, glm::vec4(1.f, 0.f, 1.f, 1.f));

	if (drawCurrentTetragon) {
		for (size_t i = 0; i < 4; i++) {
			size_t next = (i + 1 < 4 ? i + 1 : 0);
			lineBatch->line(currentTetragon[i], currentTetragon[next], glm::vec4(1.f, 0.f, 1.f, 1.f));
		}
	}
	lineBatch->lineWidth(3.f);
	lineShader->use();
	lineShader->uniformMatrix("u_projview", camera->getProjView());
	lineBatch->render();
	//glm::mat4 proj = glm::ortho(-1.f, 1.f, -1.f, 1.f, -100.0f, 100.0f);
	//glm::mat4 view = glm::lookAt(glm::vec3(2, 2, 2), glm::vec3(0.5f), glm::vec3(0, 1, 0));
	shader->use();
	shader->uniformMatrix("u_model", glm::translate(glm::mat4(1.f), glm::vec3(-1.f)));
	shader->uniformMatrix("u_proj", camera->getProjection());
	shader->uniformMatrix("u_view", camera->getView());
	shader->uniform1f("u_fogFactor", 0.f);
	shader->uniform3f("u_fogColor", glm::vec3(1.f));
	shader->uniform1f("u_fogCurve", 100.f);
	shader->uniform3f("u_cameraPos", camera->position);
	//shader->uniform3f("u_torchlightColor", glm::vec3(0.5f));
	//shader->uniform1f("u_torchlightDistance", 100.0f);
	//shader->uniform1f("u_gamma", 0.1f);
	shader->uniform1i("u_cubemap", 1);

	texture->bind();
	mesh->draw();
	Window::viewport(0, 0, Window::width, Window::height);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	framebuffer->unbind();
}
