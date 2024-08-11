#include "WorkshopPreview.hpp"

#include "../../assets/AssetsLoader.hpp"
#include "../../engine.hpp"
#include "../../graphics/core/Atlas.hpp"
#include "../../graphics/core/DrawContext.hpp"
#include "../../graphics/core/Shader.hpp"
#include "../../graphics/core/Texture.hpp"
#include "../../graphics/core/Viewport.hpp"
#include "../../graphics/ui/gui_xml.hpp"
#include "../../items/ItemDef.hpp"
#include "../../voxels/Chunk.hpp"
#include "../../voxels/ChunksStorage.hpp"
#include "../../window/Events.hpp"
#include "../../window/Window.hpp"
#include "../../world/Level.hpp"
#include "../../world/World.hpp"
#include "../ContentGfxCache.hpp"
#include "WorkshopUtils.hpp"
#include "../../voxels/Block.hpp"
#include "../../content/Content.hpp"

#include <cstring>

using namespace workshop;

vattr attr[] = { 0 };

Preview::Preview(Engine* engine, ContentGfxCache* cache) : engine(engine), cache(cache),
	blockRenderer(8192, engine->getContent(), cache, &engine->getSettings()),
	chunk(new Chunk(0, 0)),
	world(new World("", "core:default", "", 0, engine->getSettings(), engine->getContent(), engine->getContentPacks())),
	level(new Level(std::unique_ptr<World>(world), engine->getContent(), engine->getSettings())),
	camera(glm::vec3(0.f), glm::radians(60.f)),
	framebuffer(0, 0, true),
	inventory(std::make_shared<Inventory>(0, 0)),
    player(std::make_shared<Player>(level, glm::vec3(0.f), 0.f, inventory, 0)),
	controller(engine->getSettings(), std::unique_ptr<Level>(level)),
	frontend(player.get(), &controller, engine->getAssets()),
	lineBatch(1024),
	batch2d(1024),
	batch3d(1024),
	mesh(nullptr)
{
	level->chunksStorage->store(std::shared_ptr<Chunk>(chunk));
	memset(chunk->voxels, 0, sizeof(chunk->voxels));
}

void Preview::update(float delta) {
	if (!lockedKeyboardInput) {
		if (Events::pressed(keycode::D)) rotate(100.f * delta, 0.f);
		if (Events::pressed(keycode::A)) rotate(-100.f * delta, 0.f);
		if (Events::pressed(keycode::S)) rotate(0.f, 100.f * delta);
		if (Events::pressed(keycode::W)) rotate(0.f, -100.f * delta);
		if (Events::pressed(keycode::SPACE)) scale(2.f * delta);
		if (Events::pressed(keycode::LEFT_SHIFT)) scale(-2.f * delta);
	}
	if (mouseLocked) {
		rotate(-Events::delta.x / 2.f, Events::delta.y / 2.f);
		if (!Events::clicked(mousecode::BUTTON_1)) mouseLocked = false;
	}
	if (currentUI) {
		refillTimer += delta;
		if (refillTimer > 1.f) {
			refillInventory();
		}
	}
}

void Preview::updateMesh() {
	if (currentBlock == nullptr) return;
	bool rotatable = currentBlock->rotatable;
	currentBlock->rotatable = false;
	mesh = blockRenderer.render(chunk, level->chunksStorage.get());
	currentBlock->rotatable = rotatable;
}

void Preview::updateCache() {
	cache->refresh(engine->getContent(), engine->getAssets());
	updateMesh();
}

void Preview::setBlock(Block* block) {
	currentBlock = block;
	chunk->voxels[CHUNK_D * CHUNK_W + CHUNK_D + 1].id = block->rt.id;
	updateMesh();
}

void Preview::setCurrentAABB(const AABB& aabb, PrimitiveType type) {
	primitiveType = type;
	AABB& aabb_ = (type == PrimitiveType::AABB ? currentAABB : currentHitbox);
	aabb_.a = aabb.a + (aabb.b - aabb.a) / 2.f - 0.5f;
	aabb_.b = aabb.b - aabb.a;
}

void Preview::setCurrentTetragon(const glm::vec3* tetragon) {
	primitiveType = PrimitiveType::TETRAGON;
	for (size_t i = 0; i < 4; i++) {
		currentTetragon[i] = tetragon[i] - 0.5f;
	}
}

void Preview::setUiDocument(const std::shared_ptr<xml::Document> document, std::shared_ptr<int> enviroment, bool forceUpdate) {
	if (document == currentDocument && !forceUpdate) return;
	currentDocument = document;
	AssetsLoader loader(engine->getAssets(), engine->getResPaths());
	gui::UiXmlReader reader(enviroment);
	xml::xmlelement root = document->getRoot();
	currentUI = reader.readXML("", root);

	std::shared_ptr<gui::InventoryView> inventoryView = std::dynamic_pointer_cast<gui::InventoryView>(currentUI);
	if (inventoryView) {
		size_t slotsTotal = inventoryView->getSlotsCount();
		for (const auto& elem : root->getElements()) {
			const std::string& tag = elem->getTag();
			if (tag == "slots-grid") {
				size_t startIndex = 0;
				if (elem->has("start-index")) startIndex = elem->attr("start-index").asInt();
				if (elem->has("count")) slotsTotal = std::max(slotsTotal, startIndex + elem->attr("count").asInt());
				else if (elem->has("rows") && elem->has("cols")) {
					slotsTotal = std::max(slotsTotal, startIndex + elem->attr("rows").asInt() * elem->attr("cols").asInt());
				}
			}
			else if (tag == "slot") {
				if (elem->has("index")) slotsTotal = std::max(slotsTotal, static_cast<size_t>(elem->attr("index").asInt() + 1));
			}
		}
		if (!inventory || inventory->size() != slotsTotal) {
			inventory = std::make_shared<Inventory>(0, slotsTotal);
			refillInventory();
		}
		inventoryView->bind(inventory, engine->getContent());
	}
}

void Preview::setResolution(uint width, uint height) {
	framebuffer.resize(width, height);
	camera.aspect = (float)width / height;
}

void workshop::Preview::setBlockSize(glm::i8vec3 size) {
	blockSize = size;
	cameraOffset = (glm::vec3(size) + glm::vec3(0.f)) * 0.5f - 0.5f;
}

Texture* Preview::getTexture() {
	return framebuffer.getTexture();
}

void Preview::refillInventory() {
	refillTimer = 0.f;
	if (!inventory) return;
	for (size_t i = 0; i < inventory->size(); i++) {
		auto indices = cache->getContent()->getIndices();
		ItemDef* item = indices->items.get(1 + (rand() % (indices->items.count() - 1)));
		if (!item) continue;
		inventory->getSlot(i).set(ItemStack(item->rt.id, rand() % item->stackSize));
	}
}

void Preview::rotate(float x, float y) {
	previewRotation.x += x;
	previewRotation.y = std::clamp(previewRotation.y + y, -89.f, 89.f);
}

void Preview::scale(float value) {
	viewDistance = std::clamp(viewDistance + value, 1.f, 5.f);
}

void Preview::drawBlock() {
	Window::viewport(0, 0, framebuffer.getWidth(), framebuffer.getHeight());
	framebuffer.bind();
	Window::setBgColor(glm::vec4(0.f));
	Window::clear();

	Viewport viewport(Window::width, Window::height);
	DrawContext ctx(nullptr, viewport, &batch2d);

	ctx.setDepthTest(true);
	ctx.setCullFace(true);

	Assets* assets = engine->getAssets();
	Shader* lineShader = assets->get<Shader>("lines");
	Shader* shader = assets->get<Shader>("main");
	Shader* shader3d = assets->get<Shader>("ui3d");
	Texture* texture = assets->get<Atlas>("blocks")->getTexture();

	camera.rotation = glm::mat4(1.f);
	camera.rotate(glm::radians(previewRotation.y), glm::radians(previewRotation.x), 0);
	camera.position = camera.front * viewDistance;
	camera.position += cameraOffset;
	camera.dir *= -1.f;
	camera.front *= -1.f;

	if (drawGrid) {
		for (float i = -3.f; i < 3; i++) {
			lineBatch.line(glm::vec3(i + 0.5f, -0.5f, -3.f), glm::vec3(i + 0.5f, -0.5f, 3.f), glm::vec4(0.f, 0.f, 1.f, 1.f));
		}
		for (float i = -3.f; i < 3; i++) {
			lineBatch.line(glm::vec3(-3.f, -0.5f, i + 0.5f), glm::vec3(3.f, -0.5f, i + 0.5f), glm::vec4(1.f, 0.f, 0.f, 1.f));
		}
	}
	if (drawBlockSize) lineBatch.box(glm::vec3(cameraOffset), glm::vec3(blockSize), glm::vec4(1.f));
	if (drawBlockHitbox && primitiveType == PrimitiveType::HITBOX) lineBatch.box(currentHitbox.a, currentHitbox.b, glm::vec4(1.f, 1.f, 0.f, 1.f));
	if (drawCurrentAABB && primitiveType == PrimitiveType::AABB) lineBatch.box(currentAABB.a, currentAABB.b, glm::vec4(1.f, 0.f, 1.f, 1.f));

	if (drawCurrentTetragon && primitiveType == PrimitiveType::TETRAGON) {
		for (size_t i = 0; i < 4; i++) {
			size_t next = (i + 1 < 4 ? i + 1 : 0);
			lineBatch.line(currentTetragon[i], currentTetragon[next], glm::vec4(1.f, 0.f, 1.f, 1.f));
		}
	}
	if (drawDirection) {
		shader3d->use();
		shader3d->uniformMatrix("u_apply", glm::mat4(1.f));
		shader3d->uniformMatrix("u_projview", glm::translate(camera.getProjView(), glm::vec3(cameraOffset.x - 1.f, -0.98f, blockSize.z - 2.f)));
		batch3d.begin();
		batch3d.sprite(glm::vec3(1.f, 0.5f, 1.8f), glm::vec3(0.2f, 0.f, 0.f), glm::vec3(0.f, 0.f, 0.2f), 1.f, 1.f, UVRegion(), glm::vec4(0.8f));
		batch3d.point(glm::vec3(1.f, 0.5f, 2.4f), glm::vec4(0.8f));
		batch3d.point(glm::vec3(1.3f, 0.5f, 2.f), glm::vec4(0.8f));
		batch3d.point(glm::vec3(0.7f, 0.5f, 2.f), glm::vec4(0.8f));
		batch3d.flush();
	}

	lineBatch.lineWidth(3.f);
	lineShader->use();
	lineShader->uniformMatrix("u_projview", camera.getProjView());
	lineBatch.flush();
	//glm::mat4 proj = glm::ortho(-1.f, 1.f, -1.f, 1.f, -100.0f, 100.0f);
	//glm::mat4 view = glm::lookAt(glm::vec3(2, 2, 2), glm::vec3(0.5f), glm::vec3(0, 1, 0));
	shader->use();
	shader->uniformMatrix("u_model", glm::translate(glm::mat4(1.f), glm::vec3(-1.f)));
    shader->uniformMatrix("u_proj", camera.getProjection());
	shader->uniformMatrix("u_view", camera.getView());
	shader->uniform1f("u_fogFactor", 0.f);
	shader->uniform3f("u_fogColor", glm::vec3(1.f));
	shader->uniform1f("u_fogCurve", 100.f);
	shader->uniform3f("u_cameraPos", camera.position);
	//shader->uniform3f("u_torchlightColor", glm::vec3(0.5f));
	//shader->uniform1f("u_torchlightDistance", 100.0f);
	//shader->uniform1f("u_gamma", 0.1f);
	shader->uniform1i("u_cubemap", 1);

	texture->bind();
	mesh->draw();
	Window::viewport(0, 0, Window::width, Window::height);
	ctx.setDepthTest(false);
	ctx.setCullFace(false);
	framebuffer.unbind();
}

void Preview::drawUI() {
	if (!currentUI) return;
	framebuffer.bind();
	Window::setBgColor(glm::vec4(0.f));
	Window::clear();

	Viewport viewport(Window::width, Window::height);
	DrawContext ctx(nullptr, viewport, &batch2d);
	
	batch2d.begin();

	currentUI->draw(&ctx, engine->getAssets());

	framebuffer.unbind();
}