#include "WorkshopScreen.hpp"

#include "audio/audio.hpp"
#include "content/Content.hpp"
#include "content/ContentLoader.hpp"
#include "engine/Engine.hpp"
#include "files/files.hpp"
#include "graphics/core/Atlas.hpp"
#include "graphics/core/Shader.hpp"
#include "graphics/core/Texture.hpp"
#include "graphics/render/BlocksPreview.hpp"
#include "graphics/ui/elements/Menu.hpp"
#include "graphics/ui/GUI.hpp"
#include "items/ItemDef.hpp"
#include "util/stringutil.hpp"
#include "window/Camera.hpp"
#include "window/Events.hpp"
#include "window/Window.hpp"
#include "world/Level.hpp"
#include "../ContentGfxCache.hpp"
#include "IncludeCommons.hpp"
#include "menu_workshop.hpp"
#include "settings.hpp"
#include "WorkshopPreview.hpp"
#include "WorkshopSerializer.hpp"
#include "WorkshopUtils.hpp"
#include "coders/imageio.hpp"
#include "debug/Logger.hpp"
#include "objects/EntityDef.hpp"
#include "objects/rigging.hpp"
#include "graphics/commons/Model.hpp"
#include "assets/AssetsLoader.hpp"

#define NOMINMAX
#include "libs/portable-file-dialogs.h"
#include "../screens/MenuScreen.hpp"

static debug::Logger logger("workshop");

using namespace workshop;

WorkShopScreen::WorkShopScreen(Engine& engine, const ContentPack pack) : Screen(engine),
framerate(engine.getSettings().display.framerate.get()),
gui(engine.getGUI()),
currentPack(pack),
assets(engine.getAssets())
{
	gui->getMenu()->reset();
	uicamera.reset(new Camera(glm::vec3(), static_cast<float>(Window::height)));
	uicamera->perspective = false;
	uicamera->flipped = true;

	if (framerate > 60) {
		engine.getSettings().display.framerate.set(-1);
	}
	if (initialize()) {
		createNavigationPanel();
	}
	if (fs::is_regular_file(getConfigFolder(engine) / SETTINGS_FILE)) {
		std::vector<ubyte> bytes = files::read_bytes(getConfigFolder(engine) / SETTINGS_FILE);
		memcpy(&settings, bytes.data(), std::min(sizeof(settings), bytes.size()));
	}
}

WorkShopScreen::~WorkShopScreen() {
	for (const auto& elem : panels) {
		gui->remove(elem.second);
	}
	if (!fs::is_directory(getConfigFolder(engine))) fs::create_directories(getConfigFolder(engine));
	files::write_bytes(getConfigFolder(engine) / SETTINGS_FILE, reinterpret_cast<ubyte*>(&settings), sizeof(settings));
}

void WorkShopScreen::update(float delta) {
	if (Events::jpressed(keycode::ESCAPE) && (preview && !preview->lockedKeyboardInput)) {
		if (panels.size() < 2 && !showUnsaved()) {
			exit();
		}
		else if (panels.size() > 1) removePanel(panels.rbegin()->first);
		return;
	}

	for (const auto& elem : panels) {
		elem.second->UINode::setSize(glm::vec2(elem.second->getSize().x, Window::height - 4.f));
	}
	if (preview) {
		for (const auto& elem : panels) {
			if (hasFocusedTextbox(*elem.second)) {
				preview->lockedKeyboardInput = true;
				break;
			}
		}
		preview->update(delta, settings.previewSensitivity);
		preview->lockedKeyboardInput = false;
	}
}

void WorkShopScreen::draw(float delta) {
	Window::clear();
	Window::setBgColor(glm::vec3(0.2f));

	uicamera->setFov(static_cast<float>(Window::height));
	Shader* const uishader = assets->get<Shader>("ui");
	uishader->use();
	uishader->uniformMatrix("u_projview", uicamera->getProjView());

	const uint width = Window::width;
	const uint height = Window::height;

	Texture* bg = assets->get<Texture>("gui/menubg");
	batch->begin();
	batch->texture(bg);
	batch->rect(0.f, 0.f,
		width, height, 0.f, 0.f, 0.f,
		UVRegion(0.f, 0.f, width / bg->getWidth(), height / bg->getWidth()),
		false, false, glm::vec4(1.0f));
	batch->flush();
}

bool WorkShopScreen::initialize() {
	currentPackId = currentPack.id;

	auto& packs = engine.getContentPacks();
	packs.clear();

	PacksManager manager = engine.createPacksManager(engine.getPaths().getCurrentWorldFolder());
	manager.scan();
	std::vector<ContentPack> scanned = manager.getAll(manager.getAllNames());

	if (currentPackId != "base") {
		auto it = std::find_if(scanned.begin(), scanned.end(), [](const ContentPack& pack) {
			return pack.id == "base";
		});
		if (it != scanned.end()) packs.emplace_back(*it);
	}
	packs.emplace_back(currentPack);

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
		engine.loadContent();
	}
	catch (const contentpack_error& e) {
		assets = engine.getAssets();
		createContentErrorMessage(*std::find_if(packs.begin(), packs.end(), [e](const ContentPack& pack) {
			return pack.id == e.getPackId();
		}), e.what());
		return 0;
	}
	catch (const std::exception& e) {
		assets = engine.getAssets();
		createContentErrorMessage(packs.back(), e.what());
		return 0;
	}
	assets = engine.getAssets();
	content = engine.getContent();
	indices = content->getIndices();
	cache.reset(new ContentGfxCache(*content, *assets, engine.getSettings().graphics));
	previewAtlas = BlocksPreview::build(*cache, *assets, *content->getIndices()).release();
	assets->store(std::unique_ptr<Atlas>(previewAtlas), BLOCKS_PREVIEW_ATLAS);
	itemsAtlas = assets->get<Atlas>("items");
	blocksAtlas = assets->get<Atlas>("blocks");
	preview.reset(new Preview(engine, *cache));
	// force load all models and textures
	AssetsLoader loader(assets, engine.getResPaths());
	for (const auto& pack : packs){
		for (const auto& file : getFiles(pack.folder/MODELS_FOLDER, false)){
			if (assets->get<model::Model>(file.stem().string())) continue;
			if (fs::is_regular_file(file) && (file.extension() == ".obj" || file.extension() == ".vec3"))
				loader.add(AssetType::MODEL, fs::relative(file, engine.getResPaths()->getMainRoot()).replace_extension().string(), file.stem().string());
		}
		for (const auto& file : getFiles(pack.folder / TEXTURES_FOLDER / ContentPack::ENTITIES_FOLDER, false)) {
			const std::string textureName = ContentPack::ENTITIES_FOLDER.string() + '/' + file.stem().string();
			if (assets->get<Texture>(textureName)) continue;
			if (fs::is_regular_file(file) && file.extension() == ".png")
				loader.add(AssetType::TEXTURE, fs::relative(file, engine.getResPaths()->getMainRoot()).replace_extension().string(), textureName);
		}
	}
	try {
		while (loader.hasNext()) {
			loader.loadNext();
		}
	}
	catch (const assetload::error& err) {}

	backupDefs();

	return 1;
}

void WorkShopScreen::exit() {
	Engine* const e = &engine;
	e->getPaths().setCurrentWorldFolder("");
	e->getSettings().display.framerate.set(framerate);
	e->setScreen(std::make_shared<MenuScreen>(*e));
	create_workshop_button(*e, &e->getGUI()->getMenu()->getCurrent());
	e->getGUI()->getMenu()->setPage("workshop");
}

void WorkShopScreen::createNavigationPanel() {
	gui::Panel& panel = *new gui::Panel(glm::vec2(200.f));

	panels.emplace(0, std::shared_ptr<gui::Panel>(&panel));
	gui->add(panels.at(0));

	panel.setPos(glm::vec2(2.f));
	panel << new gui::Button(L"Info", glm::vec4(10.f), [this](gui::GUI*) { createPackInfoPanel(); });
	panel << new gui::Button(L"Blocks", glm::vec4(10.f), [this](gui::GUI*) { createContentList(ContentType::BLOCK); });
	panel << new gui::Button(L"Block Materials", glm::vec4(10.f), [this](gui::GUI*) { createMaterialsList(); });
	panel << new gui::Button(L"Items", glm::vec4(10.f), [this](gui::GUI*) { createContentList(ContentType::ITEM); });
	panel << new gui::Button(L"Entities", glm::vec4(10.f), [this](gui::GUI*) { createEntitiesList(); });
	panel << new gui::Button(L"Skeletons", glm::vec4(10.f), [this](gui::GUI*) { createSkeletonList(); });
	panel << new gui::Button(L"Textures", glm::vec4(10.f), [this](gui::GUI*) { createTextureList(50.f, 1, { ContentType::BLOCK, ContentType::ITEM, ContentType::ENTITY }); });
	panel << new gui::Button(L"Models", glm::vec4(10.f), [this](gui::GUI*) { createModelsList(); });
	panel << new gui::Button(L"Sounds", glm::vec4(10.f), [this](gui::GUI*) { createSoundList(); });
	panel << new gui::Button(L"Scripts", glm::vec4(10.f), [this](gui::GUI*) { createScriptList(); });
	panel << new gui::Button(L"UI Layouts", glm::vec4(10.f), [this](gui::GUI*) { createUILayoutList(); });
	gui::Button* button = new gui::Button(L"Back", glm::vec4(10.f), [this](gui::GUI*) {
		showUnsaved([this]() {
			exit();
		});
	});
	panel << button;

	createPackInfoPanel();

	gui::Container* container = new gui::Container(panel.getSize());
	container->listenInterval(0.01f, [&panel, button, container]() {
		panel.refresh();
		container->setSize(glm::vec2(panel.getSize().x, Window::height - (button->calcPos().y + (button->getSize().y + 7.f) * 4)));
	});
	panel << container;

	panel << new gui::Button(L"Utils", glm::vec4(10.f), [this](gui::GUI*) {
		createUtilsPanel();
	});
	panel << new gui::Button(L"Settings", glm::vec4(10.f), [this](gui::GUI*) {
		createSettingsPanel();
	});
	setSelectable<gui::Button>(panel);

	panel << new gui::Button(L"Open Pack Folder", glm::vec4(10.f), [this](gui::GUI*) {
		openPath(currentPack.folder);
	});
}

void WorkShopScreen::createContentErrorMessage(ContentPack& pack, const std::string& message) {
	auto panel = std::make_shared<gui::Panel>(glm::vec2(500.f, 20.f));
	panel->listenInterval(0.1f, [panel]() {
		panel->setPos(Window::size() / 2.f - panel->getSize() / 2.f);
	});

	*panel << new gui::Label("Error in content pack \"" + pack.id + "\":");
	gui::Label* label = new gui::Label(message);
	label->setMultiline(true);
	*panel << label;

	*panel << new gui::Button(L"Open pack folder", glm::vec4(10.f), [pack](gui::GUI*) {
		openPath(pack.folder);
	});
	*panel << new gui::Button(L"Back", glm::vec4(10.f), [this, panel](gui::GUI*) { gui->remove(panel); exit(); });

	panel->setPos(Window::size() / 2.f - panel->getSize() / 2.f);
	gui->add(panel);
}

void WorkShopScreen::removePanel(unsigned int column) {
	auto it = panels.find(column);
	if (it == panels.end()) return;
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

void WorkShopScreen::createPanel(const std::function<gui::Panel& ()>& lambda, unsigned int column, float posX) {
	removePanels(column);

	auto panel = std::shared_ptr<gui::Panel>(&lambda());
	panels.emplace(column, panel);

	if (posX == PANEL_POSITION_AUTO) {
		float x = 2.f;
		for (const auto& elem : panels) {
			elem.second->setPos(glm::vec2(x, 2.f));
			elem.second->act(0.1f);
			x += elem.second->getSize().x;
		}
	}
	else {
		panel->setPos(glm::vec2(posX, 2.f));
	}
	gui->add(panel);
}

void WorkShopScreen::createTexturesPanel(gui::Panel& panel, float iconSize, std::string* textures, BlockModel model,
	const std::function<void()>& callback)
{
	const char* const faces[] = { "East:", "West:", "Bottom:", "Top:", "South:", "North:" };

	size_t buttonsNum = 0;

	switch (model) {
		case BlockModel::block:
		case BlockModel::aabb: buttonsNum = 6;
			break;
		case BlockModel::xsprite: buttonsNum = 5;
			break;
		case BlockModel::custom: buttonsNum = 1;
			break;
		default: break;
	}
	if (buttonsNum == 0) return;
	panel << markRemovable(new gui::Label(buttonsNum == 1 ? L"Texture" : L"Texture faces"));
	for (size_t i = 0; i < buttonsNum; i++) {
		gui::IconButton* button = new gui::IconButton(glm::vec2(panel.getSize().x, iconSize), textures[i], blocksAtlas, textures[i],
			(buttonsNum == 6 ? faces[i] : ""));
		button->listenAction([=](gui::GUI*) {
			createTextureList(35.f, 5, { ContentType::BLOCK }, button->calcPos().x + button->getSize().x, true,
			[=](const std::string& texName) {
				textures[i] = getTexName(texName);
				removePanel(5);
				button->setColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.95f));
				button->setHoverColor(glm::vec4(0.05f, 0.1f, 0.15f, 0.75f));
				button->setIcon(getAtlas(assets, texName), getTexName(texName));
				button->setText(getTexName(texName));
				if (callback) callback();
				preview->updateCache();
			});
		});
		panel << markRemovable(button);
		if (model == BlockModel::xsprite && i == 0) i = 3;
	}

	setSelectable<gui::IconButton>(panel);
}

void WorkShopScreen::createTexturesPanel(gui::Panel& panel, float iconSize, std::string& texture, ItemIconType iconType) {
	panel.setColor(glm::vec4(0.f));
	if (iconType == ItemIconType::NONE) return;

	auto texName = [this, iconType, &texture]() {
		if (iconType == ItemIconType::SPRITE) return getTexName(texture);
		return texture;
	};

	gui::IconButton* button = new gui::IconButton(glm::vec2(panel.getSize().x, iconSize), texName(), getAtlas(assets, texture), texName());
	button->listenAction([this, button, texName, &panel, &texture, iconSize, iconType](gui::GUI*) {
		auto& nodes = button->getNodes();
		auto callback = [this, button, nodes, texName, iconSize, &texture](const std::string& textureName) {
			texture = textureName;
			removePanel(5);
			button->setIcon(getAtlas(assets, texture), texName());
			button->setText(texName());
		};
		if (iconType == ItemIconType::SPRITE) {
			createTextureList(35.f, 5, { ContentType::BLOCK, ContentType::ITEM }, PANEL_POSITION_AUTO, true, callback);
		}
		else {
			createContentList(ContentType::BLOCK, true, 5, PANEL_POSITION_AUTO, callback);
		}
		button->setColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.95f));
		button->setHoverColor(glm::vec4(0.05f, 0.1f, 0.15f, 0.75f));
	});
	panel << markRemovable(button);
}

void WorkShopScreen::createAddingUIElementPanel(float posX, const std::function<void(const std::string&)>& callback, unsigned int filter) {
	createPanel([this, callback, filter]() {
		gui::Panel& panel = *new gui::Panel(glm::vec2(200.f));

		for (const auto& elem : uiElementsArgs) {
			if (elem.second.args & UIElementsArgs::INVENTORY || filter & elem.second.args) continue;
			const std::string& name(elem.first);
			panel << new gui::Button(util::str2wstr_utf8(name), glm::vec4(10.f), [this, callback, &name](gui::GUI*) {
				callback(name);
			});
		}
		return std::ref(panel);
	}, 5, posX);
}

void WorkShopScreen::createScriptInfoPanel(const fs::path& file) {
	createPanel([this, file]() {
		gui::Panel& panel = *new gui::Panel(glm::vec2(400));

		std::string fileName(file.stem().string());
		std::string extention(file.extension().string());

		panel << new gui::Label("Path: " + fs::relative(file, currentPack.folder).remove_filename().string());
		panel << new gui::Label("File: " + fileName + extention);

		panel << new gui::Button(L"Open script", glm::vec4(10.f), [file](gui::GUI*) {
			openPath(file);
		});

		return std::ref(panel);
	}, 2);
}

void WorkShopScreen::createSoundInfoPanel(const fs::path& file) {
	createPanel([this, file]() {
		gui::Panel& panel = *new gui::Panel(glm::vec2(400));

		std::string fileName(file.stem().string());
		std::string extention(file.extension().string());

		panel << new gui::Label("Path: " + fs::relative(file, currentPack.folder).remove_filename().string());
		panel << new gui::Label("File: " + fileName + extention);

		fs::path parentDir = fs::relative(file.parent_path(), currentPack.folder / SOUNDS_FOLDER);

		//audio::Sound* sound = assets->get<audio::Sound>(parentDir.empty() ? "" : parentDir.string() + "/" + fileName);
		//sound->newInstance(0, 0);

		return std::ref(panel);
	}, 2);
}

void workshop::WorkShopScreen::createSettingsPanel() {
	createPanel([this]() {
		gui::Panel& panel = *new gui::Panel(glm::vec2(300));

		panel << new gui::Label(L"Custom model input range");
		panel << createNumTextBox(settings.customModelRange.x, L"Min", -FLT_MAX, FLT_MAX);
		panel << createNumTextBox(settings.customModelRange.y, L"Max", -FLT_MAX, FLT_MAX);

		panel << new gui::Label(L"Content list width");
		panel << createNumTextBox(settings.contentListWidth, L"", 100.f, 1000.f);
		panel << new gui::Label(L"Block editor width");
		panel << createNumTextBox(settings.blockEditorWidth, L"", 100.f, 1000.f);
		panel << new gui::Label(L"Custom model editor width");
		panel << createNumTextBox(settings.customModelEditorWidth, L"", 100.f, 1000.f);
		panel << new gui::Label(L"Item editor width");
		panel << createNumTextBox(settings.itemEditorWidth, L"", 100.f, 1000.f);
		panel << new gui::Label(L"Texture list width");
		panel << createNumTextBox(settings.textureListWidth, L"", 100.f, 1000.f);

		gui::Label& label = *new gui::Label(L"Preview window sensitivity: " + util::to_wstring(settings.previewSensitivity, 2));
		panel << label;
		gui::TrackBar& trackBar = *new gui::TrackBar(0.1, 5.0, settings.previewSensitivity, 0.01);
		trackBar.setConsumer([this, &label](double num) {
			settings.previewSensitivity = num;
			label.setText(L"Preview window sensitivity: " + util::to_wstring(settings.previewSensitivity, 2));
		});
		panel << trackBar;

		return std::ref(panel);
	}, 1);
}

void workshop::WorkShopScreen::createUtilsPanel() {
	createPanel([this]() {
		gui::Panel& panel = *new gui::Panel(glm::vec2(300));

		panel << new gui::Button(L"Fix aabb's", glm::vec4(10.f), [this](gui::GUI*) {
			std::unordered_map<Block*, size_t> brokenAABBs;
			for (size_t i = 0; i < indices->blocks.count(); i++) {
				size_t aabbsNum = 0;
				Block* block = indices->blocks.getIterable().at(i);
				if (block->customModelRaw.has(AABB_STR)) {
					for (const dv::value& elem : block->customModelRaw[AABB_STR].asList()) {
						AABB aabb = exportAABB(elem);
						if (aabb.a != aabb.min() && aabb.b != aabb.max()) aabbsNum++;
					}
				}
				if (aabbsNum > 0) brokenAABBs.emplace(block, aabbsNum);
			}
			createPanel([this, brokenAABBs]() {
				gui::Panel& panel = *new gui::Panel(glm::vec2(300));

				if (brokenAABBs.empty()) {
					panel << new gui::Label(L"Wrong aabb's not found");
				}
				else {
					panel << new gui::Button(L"Fix all", glm::vec4(10.f), [this, brokenAABBs](gui::GUI*) {
						for (const auto& pair : brokenAABBs) {
							for (dv::value& aabb : pair.first->customModelRaw[AABB_STR]) {
								AABB broken(exportAABB(aabb));
								broken.b += broken.a;
								glm::vec3 fixed[2]{ broken.min(), broken.max() };
								fixed[1] -= fixed[0];
								for (size_t i = 0; i < 2; i++) {
									for (size_t j = 0; j < 3; j++) {
										aabb[i * 3 + j] = fixed[i][j];
									}
								}
							}
						}
						preview->updateCache();
						removePanel(2);
					});
					for (const auto& pair : brokenAABBs) {
						panel << new gui::Label("Block: " + getDefName(pair.first->name));
						panel << new gui::Label("Wrong aabb's: " + std::to_string(pair.second));
					}
				}

				return std::ref(panel);
			}, 2);
		});

		panel << new gui::Button(L"Find unused textures", glm::vec4(10.f), [this](gui::GUI*) {
			const fs::path blocksTexturesPath(currentPack.folder / TEXTURES_FOLDER / ContentPack::BLOCKS_FOLDER);
			const fs::path itemsTexturesPath(currentPack.folder / TEXTURES_FOLDER / ContentPack::ITEMS_FOLDER);

			std::unordered_multimap<ContentType, fs::path> unusedTextures;

			for (const auto& path : getFiles(blocksTexturesPath, false)) {
				if (!fs::is_regular_file(path)) continue;
				const std::string fileName = path.stem().string();
				bool inUse = false;
				for (size_t i = 0; i < indices->blocks.count() && !inUse; i++) {
					Block* const block = const_cast<Block*>(indices->blocks.get(i));
					std::vector<dv::value*> strings = getAllWithType(block->customModelRaw, dv::value_type::string);
					if (block->name.find(currentPackId) == std::string::npos) continue;
					if (std::find(std::begin(block->textureFaces), std::end(block->textureFaces), fileName) != std::end(block->textureFaces)) inUse = true;
					if (std::find_if(strings.begin(), strings.end(), [fileName](const dv::value* value){
						return value->asString() == fileName;
					}) != strings.end()) inUse = true;
				}
				for (size_t i = 0; i < indices->items.count() && !inUse; i++) {
					const ItemDef* const item = indices->items.get(i);
					if (item->name.find(currentPackId) == std::string::npos) continue;
					if (item->iconType == ItemIconType::SPRITE && getTexName(item->icon) == fileName) inUse = true;
				}
				if (!inUse) unusedTextures.emplace(ContentType::BLOCK, path);
			}
			for (const auto& path : getFiles(itemsTexturesPath, false)) {
				const std::string fileName = path.stem().string();
				bool inUse = false;
				for (size_t i = 0; i < indices->items.count() && !inUse; i++) {
					const ItemDef* const item = indices->items.get(i);
					if (item->iconType == ItemIconType::SPRITE && getTexName(item->icon) == fileName) inUse = true;
				}
				if (!inUse) unusedTextures.emplace(ContentType::ITEM, path);
			}
			createPanel([this, unusedTextures]() {
				gui::Panel& panel = *new gui::Panel(glm::vec2(300));

				if (unusedTextures.empty()) {
					panel << new gui::Label("Unused textures not found");
				}
				else {
					panel << new gui::Label("Found " + std::to_string(unusedTextures.size()) + " unused texture(s)");
					panel << new gui::Button(L"Delete all", glm::vec4(10.f), [this, unusedTextures](gui::GUI*) {
						std::vector<fs::path> v;
						std::transform(unusedTextures.begin(), unusedTextures.end(), std::back_inserter(v), [](const auto& pair) {return pair.second; });
						createFileDeletingConfirmationPanel(v, 3, [this]() {
							initialize();
							createUtilsPanel();
						});
					});
					for (const auto& pair : unusedTextures) {
						const Atlas* atlas = assets->get<Atlas>(getDefFolder(pair.first));
						const std::string file = pair.second.stem().string();

						gui::IconButton& button = *new gui::IconButton(glm::vec2(panel.getSize().x, 50.f), file, atlas, file);
						button.listenAction([this, file, pair](gui::GUI*) {
							createTextureInfoPanel(getDefFolder(pair.first) + ':' + file, pair.first, 3);
						});
						panel << button;
					}
				}

				return std::ref(panel);
			}, 2);
		});

		panel << new gui::Button(L"Find texture duplicates", glm::vec4(10.f), [this](gui::GUI*) {
			const fs::path blocksTexturesPath(currentPack.folder / TEXTURES_FOLDER / ContentPack::BLOCKS_FOLDER);
			const Texture* const blocksTexture = blocksAtlas->getTexture();
			ubyte* imageData = blocksAtlas->getImage()->getData();

			std::unordered_map<fs::path, std::set<fs::path>> duplicates;

			std::vector<fs::path> files = getFiles(blocksTexturesPath, false);
			std::sort(files.begin(), files.end(), [](const fs::path& a, const fs::path& b) {
				return a.stem().string() < b.stem().string();
			});

			std::vector<std::vector<ubyte>> filesBytes;
			for (const auto& file : files) {
				filesBytes.emplace_back(files::read_bytes(file));
			}
			std::set<fs::path> skipList;

			for (size_t i = 0; i < filesBytes.size(); i++) {
				for (size_t j = i + 1; j < filesBytes.size(); j++) {
					if (skipList.find(files[j]) != skipList.end()) continue;
					if (filesBytes[i] == filesBytes[j]) {
						if (duplicates.find(files[i]) == duplicates.end()) {
							duplicates.emplace(files[i], std::set<fs::path>{files[j]});
						}
						else {
							duplicates.at(files[i]).insert(files[j]);
						}
						skipList.insert(files[j]);
					}
				}
			}
			createPanel([this, duplicates]() {
				gui::Panel& panel = *new gui::Panel(glm::vec2(300));

				std::vector<fs::path> files;

				if (duplicates.empty()) {
					panel << new gui::Label("Duplicated textures not found");
				}
				else {
					gui::Panel& filesList = *new gui::Panel(panel.getSize());
					filesList.setMaxLength(500);

					for (const auto& pair : duplicates) {
						filesList << new gui::Label(pair.first.stem().string());
						for (const auto& duplicate : pair.second) {
							std::string file = duplicate.stem().string();
							files.emplace_back(duplicate);
							filesList << new gui::IconButton(glm::vec2(panel.getSize().x, 50.f), file, blocksAtlas, file);
						}
					}
					optimizeContainer(filesList);
					panel << new gui::Label("Found " + std::to_string(files.size()) + " duplicated textures");
					panel << filesList;
					panel << new gui::Button(L"Delete duplicates", glm::vec4(10.f), [this, duplicates, files](gui::GUI*) {
						for (const auto& pair : duplicates) {
							const std::string uniqueTexName = pair.first.stem().string();
							for (const auto& duplicate : pair.second) {
								const std::string duplicateTexName = duplicate.stem().string();
								for (size_t i = 0; i < indices->blocks.count(); i++) {
									Block* const block = indices->blocks.getIterable().at(i);
									if (block->name.find(currentPackId) == std::string::npos) continue;
									std::vector<dv::value*> strings = getAllWithType(block->customModelRaw, dv::value_type::string);
									for (dv::value* blockTexture : strings) {
										if (blockTexture->asString() == duplicateTexName) *blockTexture = uniqueTexName;
									}
									for (std::string& blockTexture : block->textureFaces) {
										if (blockTexture == duplicateTexName) blockTexture = uniqueTexName;
									}
								}
							}
						}
						createFileDeletingConfirmationPanel(files, 3, [this]() {
							initialize();
							createUtilsPanel();
						});
					});
				}

				return std::ref(panel);
			}, 2);
		});

		return std::ref(panel);
	}, 1);
}

void WorkShopScreen::createTextureInfoPanel(const std::string& texName, ContentType type, unsigned int column) {
	createPanel([this, texName, type]() {
		gui::Panel& panel = *new gui::Panel(glm::vec2(350));

		panel << new gui::Label(texName);

		Texture* texture = nullptr;
		UVRegion uv;
		if (type == ContentType::ENTITY){
			texture = assets->get<Texture>(texName);
		}
		else{
			const Atlas* const atlas = getAtlas(assets, texName);
			texture = atlas->getTexture();
			uv = atlas->get(getTexName(texName));
		}

		gui::Image& image = *new gui::Image(texture, glm::vec2(0.f));
		formatTextureImage(image, texture, panel.getSize().x, uv);
		gui::Container& imageContainer = *new gui::Container(glm::vec2(panel.getSize().x));
		imageContainer << image;
		panel << imageContainer;

		const glm::ivec2 size((uv.u2 - uv.u1) * texture->getWidth(), (uv.v2 - uv.v1) * texture->getHeight());
		panel << new gui::Label(L"Width: " + std::to_wstring(size.x));
		panel << new gui::Label(L"Height: " + std::to_wstring(size.y));
		panel << new gui::Label(L"Texture type: " + util::str2wstr_utf8(getDefName(type)));
		panel << new gui::Button(L"Delete", glm::vec4(10.f), [this, texName, type](gui::GUI*) {
			showUnsaved([this, texName, type]() {
				createFileDeletingConfirmationPanel({ currentPack.folder / TEXTURES_FOLDER / getDefFolder(type) / std::string(getTexName(texName) + ".png") },
				3, [this]() {
					initialize();
					createTextureList(50.f, 1);
				});
			});
		});
		return std::ref(panel);
	}, column);
}

void WorkShopScreen::createImportPanel(ContentType type, std::string mode) {
	createPanel([this, type, mode]() {
		gui::Panel& panel = *new gui::Panel(glm::vec2(200));

		panel << new gui::Label(std::wstring(L"Import ") + (type == ContentType::MODEL ? L"model" : L"texture"));
		if (type != ContentType::MODEL) {
			panel << new gui::Button(L"Import as: " + util::str2wstr_utf8(getDefName(type)), glm::vec4(10.f), [this, type, mode](gui::GUI*) {
				switch (type) {
					case ContentType::BLOCK: createImportPanel(ContentType::ITEM, mode); break;
					case ContentType::ITEM: createImportPanel(ContentType::ENTITY, mode); break;
					case ContentType::ENTITY: createImportPanel(ContentType::BLOCK, mode); break;
				}
			});
		}
		panel << new gui::Button(L"Import mode: " + util::str2wstr_utf8(mode), glm::vec4(10.f), [this, type, mode](gui::GUI*) {
			if (mode == "copy") createImportPanel(type, "move");
			else if (mode == "move") createImportPanel(type, "copy");
		});
		panel << new gui::Button(L"Select files", glm::vec4(10.f), [this, type, mode](gui::GUI*) {
			showUnsaved([this, type, mode]() {
				const std::string fileFormat = type == ContentType::MODEL ? getDefFileFormat(ContentType::MODEL) : ".png";
				auto files = pfd::open_file("", "", { '(' + fileFormat + ')', '*' + fileFormat }, pfd::opt::multiselect).result();
				if (files.empty()) return;
				fs::path path(currentPack.folder / (type == ContentType::MODEL ? "" : TEXTURES_FOLDER) / getDefFolder(type));
				if (!fs::is_directory(path)) fs::create_directories(path);
				for (const auto& elem : files) {
					if (fs::is_regular_file(path / fs::path(elem).filename())) continue;
					fs::copy(elem, path);
					if (mode == "move") fs::remove(elem);
				}
				initialize();
				if (type == ContentType::MODEL) createModelsList();
				else createTextureList(50.f);
			});
		});

		return std::ref(panel);
	}, 2);
}

void workshop::WorkShopScreen::createFileDeletingConfirmationPanel(const std::vector<fs::path>& files, unsigned int column,
	const std::function<void(void)>& callback)
{
	createPanel([this, files, column, callback]() {
		gui::Panel& panel = *new gui::Panel(glm::vec2(300));

		gui::Label& label = *new gui::Label("Are you sure you want to permanently delete this file(s)?");
		label.setTextWrapping(true);
		label.setMultiline(true);
		label.setSize(panel.getSize() / 4.f);
		panel << label;

		gui::Panel& filesList = *new gui::Panel(panel.getSize());
		filesList.setMaxLength(300);
		for (const auto& file : files) {
			filesList << new gui::Label(fs::relative(file, currentPack.folder).string());
		}
		panel << filesList;

		panel << new gui::Button(L"Confirm", glm::vec4(10.f), [this, files, column, callback](gui::GUI*) {
			if (showUnsaved([this, files, column, callback]() {
				for (const auto& file : files) {
					fs::remove(file);
				}
				removePanel(column);
				if (callback) callback();
			}));
		});
		panel << new gui::Button(L"Cancel", glm::vec4(10.f), [this, column](gui::GUI*) {
			removePanel(column);
		});

		return std::ref(panel);
	}, column);
}

void WorkShopScreen::createMaterialEditor(BlockMaterial& material) {
	createPanel([this, &material]() {
		gui::Panel& panel = *new gui::Panel(glm::vec2(200));

		panel << new gui::Label(material.name);

		panel << new gui::Label("Step Sound");
		createTextBox(panel, material.stepsSound);
		panel << new gui::Label("Place Sound");
		createTextBox(panel, material.placeSound);
		panel << new gui::Label("Break Sound");
		createTextBox(panel, material.breakSound);

		return std::ref(panel);
	}, 2);
}

void workshop::WorkShopScreen::createPreview(unsigned int column, const std::function<void(gui::Panel&, gui::Image&)>& setupFunc) {
	createPanel([this, setupFunc]() {
		gui::Panel& panel = *new gui::Panel(glm::vec2(300));
		gui::Image& image = *new gui::Image(preview->getTexture(), glm::vec2(panel.getSize().x));
		panel << image;
		panel.listenInterval(0.01f, [this, &panel, &image]() {
			if (panel.isHover() && image.isInside(Events::cursor)) {
				if (Events::jclicked(mousecode::BUTTON_1)) preview->lmb = true;
				if (Events::jclicked(mousecode::BUTTON_2)) preview->rmb = true;
				if (Events::scroll) preview->scale(-Events::scroll / 10.f);
			}
			panel.setSize(glm::vec2(Window::width - panel.calcPos().x - 2.f, Window::height));
			image.setSize(glm::vec2(panel.getSize().x, Window::height / 2.f));
			preview->setResolution(static_cast<uint>(image.getSize().x), static_cast<uint>(image.getSize().y));
			image.setTexture(preview->getTexture());
		});
		createFullCheckBox(panel, L"Draw grid", preview->drawGrid);
		createFullCheckBox(panel, L"Show front direction", preview->drawDirection);
		setupFunc(panel, image);
		panel << new gui::Button(L"Take screenshot", glm::vec4(10.f), [this](gui::GUI*) {
			auto data = preview->getTexture()->readData();
			data->flipY();
			fs::path filename = engine.getPaths().getNewScreenshotFile("png");
			imageio::write(filename.string(), data.get());
			logger.info() << "saved screenshot as " << filename.u8string();
		});
		panel << new gui::Button(L"Reset view", glm::vec4(10.f), [this](gui::GUI*) {
			preview->resetView();
		});

		panel << new gui::Label("");
		panel << new gui::Label("Camera control");
		panel << new gui::Label("Position: Left Control + W, S, A, D or RMB + mouse");
		panel << new gui::Label("Rotate: W, S, A, D or LMB + mouse");
		panel << new gui::Label("Zoom: Left Shift/Space or Scroll Wheel");

		return std::ref(panel);
	}, column);
}

void WorkShopScreen::createBlockPreview(unsigned int column, PrimitiveType type, Block& block) {
	gui::Panel* panelPtr = nullptr;
	gui::Label* lookAtInfo = new gui::Label("");
	lookAtInfo->setMultiline(true);
	lookAtInfo->setSize(lookAtInfo->getSize() * 4.f);
	lookAtInfo->setLineInterval(1.2f);
	createPreview(column, [this, type, pp = &panelPtr, lookAtInfo, &block](gui::Panel& panel, gui::Image& image) {
		*pp = &panel;
		panel.listenInterval(0.01f, [this, type, lookAtInfo, &panel, &image, &block]() {
			size_t index;
			preview->lookAtPrimitive = PrimitiveType::COUNT;
			if (type != PrimitiveType::HITBOX) lookAtInfo->setText(L"\nYou can select a primitive with the mouse\n\n");
			if (panel.isHover() && image.isInside(Events::cursor)) {
				if (type != PrimitiveType::HITBOX && preview->rayCast(Events::cursor.x - image.calcPos().x, Events::cursor.y - image.calcPos().y, index)) {
					if (Events::jclicked(mousecode::BUTTON_1)) {
						createCustomModelEditor(block, index, preview->lookAtPrimitive);
					}
					lookAtInfo->setText(L"\nPointing at: " + getPrimitiveName(preview->lookAtPrimitive) + 
						L"\nPrimitive index: " + std::to_wstring(index) + L"\nClick LMB to choose");
				}
			}
			preview->drawBlock();
		});
		createFullCheckBox(panel, L"Draw block size", preview->drawBlockSize);
		if (type == PrimitiveType::HITBOX)
			createFullCheckBox(panel, L"Draw current hitbox", preview->drawBlockHitbox);
		else if (type == PrimitiveType::AABB)
			createFullCheckBox(panel, L"Highlight current AABB", preview->drawCurrentAABB);
		else if (type == PrimitiveType::TETRAGON)
			createFullCheckBox(panel, L"Highlight current Tetragon", preview->drawCurrentTetragon);
	});
	*panelPtr << lookAtInfo;
}

void workshop::WorkShopScreen::createSkeletonPreview(unsigned int column) {
	createPreview(column, [this](gui::Panel& panel, gui::Image& image) {
		panel.listenInterval(0.01f, [this]() {
			preview->drawSkeleton();
		});
	});
}

void workshop::WorkShopScreen::createModelPreview(unsigned int column) {
	createPreview(column, [this](gui::Panel& panel, gui::Image& image) {
		panel.listenInterval(0.01f, [this]() {
			preview->drawModel();
		});
	});
}

void WorkShopScreen::createUIPreview() {
	createPanel([this]() {
		gui::Panel& panel = *new gui::Panel(glm::vec2(300));
		gui::Image& image = *new gui::Image(preview->getTexture(), glm::vec2(panel.getSize().x, Window::height));
		panel << image;
		panel.listenInterval(0.01f, [this, &panel, &image]() {
			panel.setSize(glm::vec2(Window::width - panel.calcPos().x - 2.f, Window::height));
			image.setSize(glm::vec2(image.getSize().x, Window::height));
			preview->setResolution(Window::width, Window::height);
			image.setTexture(preview->getTexture());
			image.setUVRegion(UVRegion(0.f, 0.f, image.getSize().x / Window::width, 1.f));
			preview->drawUI();
		});

		return std::ref(panel);
	}, 3);
}

static std::string findParent(const fs::path& path, workshop::ContentType type, const std::string& actualName) {
	std::string parentName;
	fs::path file(path / getDefFolder(type) / (actualName + getDefFileFormat(type)));
	if (!fs::is_regular_file(file)) return parentName;
	dv::value map = files::read_json(file);
	if (map.has("parent")) parentName = map.asString("parent");
	return parentName;
}

template<typename T>
static void backup(std::unordered_map<std::string, BackupData>& dst, const ContentUnitIndices<T>& src, const ContentUnitDefs<T>& content,
	workshop::ContentType type, const std::string& packId, fs::path& packPath
) {
	for (size_t i = 0; i < src.count(); i++) {
		const T* const def = src.get(i);
		if (def->name.find(packId) == std::string::npos) continue;
		std::string defName(getDefName(def->name));
		BackupData& data = dst[defName];
		data.currentParent = data.newParent = findParent(packPath, type, defName);
		data.string = stringify(toJson(*def, defName, content.find(data.currentParent), data.newParent), false);
	}
}

void workshop::WorkShopScreen::backupDefs() {
	blocksList.clear();
	itemsList.clear();
	entityList.clear();
	skeletons.clear();
	backup<Block>(blocksList, indices->blocks, content->blocks, ContentType::BLOCK, currentPackId, currentPack.folder);
	backup<ItemDef>(itemsList, indices->items, content->items, ContentType::ITEM, currentPackId, currentPack.folder);
	backup<EntityDef>(entityList, indices->entities, content->entities, ContentType::ENTITY, currentPackId, currentPack.folder);
	for (const auto& [name, skeleton] : content->getSkeletons()){
		if (name.find(currentPackId) != 0) continue;
		skeletons[getDefName(name)] = stringify(toJson(*skeleton->getRoot(), getDefName(name)), false);
	}
}

bool workshop::WorkShopScreen::showUnsaved(const std::function<void()>& callback) {
	std::unordered_multimap<ContentType, std::string> unsavedList;

	for (const auto& [name, data] : blocksList) {
		const Block* parent = content->blocks.find(data.currentParent);
		if (data.string != stringify(toJson(*content->blocks.find(currentPackId + ':' + name), name, parent, data.newParent), false))
			unsavedList.emplace(ContentType::BLOCK, name);
	}
	for (const auto& [name, data] : itemsList) {
		const ItemDef* item = content->items.find(currentPackId + ':' + name);
		if (item->generated) continue;
		const ItemDef* parent = content->items.find(data.currentParent);
		if (data.string != stringify(toJson(*item, name, parent, data.newParent), false))
			unsavedList.emplace(ContentType::ITEM, name);
	}
	for (const auto& [name, data] : entityList) {
		const EntityDef* parent = content->entities.find(data.currentParent);
		if (data.string != stringify(toJson(*content->entities.find(currentPackId + ':' + name), name, parent, data.newParent), false))
			unsavedList.emplace(ContentType::ENTITY, name);
	}
	for (const auto& [name, data] : skeletons){
		if (data != stringify(toJson(*content->getSkeleton(currentPackId + ':' + name)->getRoot(), name), false))
			unsavedList.emplace(ContentType::SKELETON, name);
	}

	if (unsavedList.empty() || ignoreUnsaved) {
		ignoreUnsaved = false;
		if (callback) callback();
		return false;
	}
	gui::Container& container = *new gui::Container(Window::size());
	std::shared_ptr<gui::Container> containerPtr(&container);
	container.setColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.75f));

	gui::Panel& outerPanel = *new gui::Panel(Window::size() / 2.f);
	outerPanel.setColor(glm::vec4(0.5f, 0.5f, 0.5f, 0.25f));
	outerPanel.listenInterval(0.01f, [this, &outerPanel, &container]() {
		preview->lockedKeyboardInput = true;
		outerPanel.setPos(Window::size() / 2.f - outerPanel.getSize() / 2.f);
		container.setSize(Window::size());
	});
	outerPanel << new gui::Label("You have " + std::to_string(unsavedList.size()) + " unsaved change(s):");

	gui::Panel& innerPanel = *new gui::Panel(glm::vec2());
	innerPanel.setColor(glm::vec4(0.f));
	innerPanel.setMaxLength(Window::height / 3.f);

	for (const auto& elem : unsavedList) {
		innerPanel << new gui::Label(getDefName(elem.first) + ": " + elem.second);
	}

	outerPanel << innerPanel;
	gui::Container& buttonsContainer = *new gui::Container(glm::vec2(outerPanel.getSize().x, 20));
	buttonsContainer << new gui::Button(L"Back", glm::vec4(10.f), [this, containerPtr](gui::GUI*) { gui->remove(containerPtr); });
	buttonsContainer << new gui::Button(L"Save all", glm::vec4(10.f), [this, containerPtr, unsavedList, callback](gui::GUI*) {
		for (const auto& pair : unsavedList) {
			const BackupData& backup = (pair.first == ContentType::BLOCK ? blocksList.at(pair.second) : itemsList.at(pair.second));
			if (pair.first == ContentType::BLOCK)
				saveBlock(*content->blocks.find(currentPackId + ':' + pair.second), currentPack.folder, pair.second,
					content->blocks.find(backup.currentParent), backup.newParent);
			else if (pair.first == ContentType::ITEM)
				saveItem(*content->items.find(currentPackId + ':' + pair.second), currentPack.folder, pair.second,
					content->items.find(backup.currentParent), backup.newParent);
			else if (pair.first == ContentType::ENTITY)
				saveEntity(*content->entities.find(currentPackId + ':' + pair.second), currentPack.folder, pair.second,
					content->entities.find(backup.currentParent), backup.newParent);
			else if (pair.first == ContentType::SKELETON)
				saveSkeleton(*content->getSkeleton(pair.second)->getRoot(), currentPack.folder, pair.second);
		}
		backupDefs();
		gui->remove(containerPtr);
		if (callback) callback();
	});
	buttonsContainer << new gui::Button(L"Discard all", glm::vec4(10.f), [this, containerPtr](gui::GUI*) {
		initialize();
		removePanels(0);
		createNavigationPanel();
		gui->remove(containerPtr);
	});
	buttonsContainer << new gui::Button(L"Ignore once", glm::vec4(10.f), [this, containerPtr, callback](gui::GUI*) {
		ignoreUnsaved = true;
		gui->remove(containerPtr);
		if (callback) callback();
	});
	placeNodesHorizontally(buttonsContainer);
	outerPanel << buttonsContainer;
	outerPanel.setPos(Window::size() / 2.f - outerPanel.getSize() / 2.f);

	container << outerPanel;
	gui->add(containerPtr);

	return true;
}