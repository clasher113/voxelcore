#include "WorkshopSerializer.hpp"

#include "../../coders/json.hpp"
#include "../../coders/xml.hpp"
#include "../../content/ContentPack.hpp"
#include "../../data/dynamic.hpp"
#include "../../files/files.hpp"
#include "../../items/ItemDef.hpp"
#include "../../voxels/Block.hpp"

#include <algorithm>

void workshop::saveContentPack(const ContentPack& pack) {
	dynamic::Map root;
	root.put("id", pack.id);
	root.put("title", pack.title);
	root.put("version", pack.version);
	root.put("creator", pack.creator);
	root.put("description", pack.description);

	if (!pack.dependencies.empty()) {
		auto& dependencies = root.putList("dependencies");
		for (const auto& elem : pack.dependencies) {
			if (!elem.id.empty()) dependencies.put(elem.id);
		}
	}
	files::write_json(pack.folder / ContentPack::PACKAGE_FILENAME, &root);
}

void workshop::saveBlock(const Block& block, const std::filesystem::path& packFolder, const std::string& actualName) {
	Block temp("");
	dynamic::Map root;
	if (block.model != BlockModel::custom) {
		if (std::all_of(std::cbegin(block.textureFaces), std::cend(block.textureFaces), [&r = block.textureFaces[0]](const std::string& value) {return value == r; }) ||
			block.model == BlockModel::xsprite) {
			root.put("texture", block.textureFaces[0]);
		}
		else {
			auto& texarr = root.putList("texture-faces");
			for (size_t i = 0; i < 6; i++) {
				texarr.put(block.textureFaces[i]);
			}
		}
	}

	if (temp.lightPassing != block.lightPassing) root.put("light-passing", block.lightPassing);
	if (temp.skyLightPassing != block.skyLightPassing) root.put("sky-light-passing", block.skyLightPassing);
	if (temp.obstacle != block.obstacle) root.put("obstacle", block.obstacle);
	if (temp.selectable != block.selectable) root.put("selectable", block.selectable);
	if (temp.replaceable != block.replaceable) root.put("replaceable", block.replaceable);
	if (temp.breakable != block.breakable) root.put("breakable", block.breakable);
	if (temp.grounded != block.grounded) root.put("grounded", block.grounded);
	if (temp.drawGroup != block.drawGroup) root.put("draw-group", static_cast<int64_t>(block.drawGroup));
	if (temp.hidden != block.hidden) root.put("hidden", block.hidden);

	if (block.pickingItem != block.name + BLOCK_ITEM_SUFFIX) {
		root.put("picking-item", block.pickingItem);
	}

	const char* models[] = { "none", "block", "X", "aabb", "custom" };
	if (temp.model != block.model) root.put("model", models[static_cast<size_t>(block.model)]);
	if (block.emission[0] || block.emission[1] || block.emission[2]) {
		auto& emissionarr = root.putList("emission");
		emissionarr.multiline = false;
		for (size_t i = 0; i < 3; i++) {
			emissionarr.put(static_cast<int64_t>(block.emission[i]));
		}
	}
	if (block.rotatable) {
		root.put("rotation", block.rotations.name);
	}
	auto putVec3 = [](dynamic::List& list, const glm::vec3& vector) {
		list.put(vector.x); list.put(vector.y); list.put(vector.z);
	};
	auto putAABB = [putVec3](dynamic::List& list, AABB aabb) {
		aabb.b -= aabb.a;
		putVec3(list, aabb.a); putVec3(list, aabb.b);
	};
	if (block.hitboxes.size() == 1) {
		const AABB& hitbox = block.hitboxes.front();
		AABB aabb;
		if (aabb.a != hitbox.a || aabb.b != hitbox.b) {
			auto& boxarr = root.putList("hitbox");
			boxarr.multiline = false;
			putAABB(boxarr, hitbox);
		}
	}
	else if (!block.hitboxes.empty()) {
		auto& hitboxesArr = root.putList("hitboxes");
		for (const auto& hitbox : block.hitboxes) {
			auto& hitboxArr = hitboxesArr.putList();
			hitboxArr.multiline = false;
			putAABB(hitboxArr, hitbox);
		}
	}

	auto isElementsEqual = [](const std::vector<std::string>& arr, size_t offset, size_t numElements) {
		return std::all_of(std::cbegin(arr) + offset, std::cbegin(arr) + offset + numElements, [&r = arr[offset]](const std::string& value) {return value == r; });
	};
	if (block.model == BlockModel::custom) {
		dynamic::Map& model = root.putMap("model-primitives");
		size_t offset = 0;
		if (!block.modelBoxes.empty()) {
			dynamic::List& aabbs = model.putList("aabbs");
			for (size_t i = 0; i < block.modelBoxes.size(); i++) {
				dynamic::List& aabb = aabbs.putList();
				aabb.multiline = false;
				putAABB(aabb, block.modelBoxes[i]);
				size_t textures = (isElementsEqual(block.modelTextures, offset, 6) ? 1 : 6);
				for (size_t j = 0; j < textures; j++) {
					aabb.put(block.modelTextures[offset + j]);
				}
				offset += 6;
			}
		}
		if (!block.modelExtraPoints.empty()) {
			dynamic::List& tetragons = model.putList("tetragons");
			for (size_t i = 0; i < block.modelExtraPoints.size(); i += 4) {
				dynamic::List& tetragon = tetragons.putList();
				tetragon.multiline = false;
				putVec3(tetragon, block.modelExtraPoints[i]);
				putVec3(tetragon, block.modelExtraPoints[i + 1] - block.modelExtraPoints[i]);
				putVec3(tetragon, block.modelExtraPoints[i + 3] - block.modelExtraPoints[i]);
				tetragon.put(block.modelTextures[offset++]);
			}
		}
	}
	if (block.scriptName != actualName) root.put("script-name", block.scriptName);
	if (block.material != DEFAULT_MATERIAL) root.put("material", block.material);
	if (block.inventorySize != 0) root.put("inventory-size", static_cast<int64_t>(block.inventorySize));
	if (block.uiLayout != block.name) root.put("ui-layout", block.uiLayout);

	json::precision = 3;
	std::filesystem::path path = packFolder / ContentPack::BLOCKS_FOLDER;
	if (!std::filesystem::is_directory(path)) std::filesystem::create_directories(path);
	files::write_json(path / std::filesystem::path(actualName + ".json"), &root);
	json::precision = 15;
}

void workshop::saveItem(const ItemDef& item, const std::filesystem::path& packFolder, const std::string& actualName) {
	ItemDef temp("");
	dynamic::Map root;
	if (temp.stackSize != item.stackSize) root.put("stack-size", static_cast<int64_t>(item.stackSize));
	if (item.emission[0] || item.emission[1] || item.emission[2]) {
		auto& emissionarr = root.putList("emission");
		emissionarr.multiline = false;
		for (size_t i = 0; i < 3; i++) {
			emissionarr.put(static_cast<int64_t>(item.emission[i]));
		}
	}
	std::string iconStr("");
	switch (item.iconType) {
	case item_icon_type::none: iconStr = "none";
		break;
	case item_icon_type::block: iconStr = "block";
		break;
	}
	if (!iconStr.empty()) root.put("icon-type", iconStr);
	if (temp.icon != item.icon) root.put("icon", item.icon);
	if (temp.placingBlock != item.placingBlock) root.put("placing-block", item.placingBlock);
	if (item.scriptName != actualName) root.put("script-name", item.scriptName);

	fs::path path = packFolder / ContentPack::ITEMS_FOLDER;
	if (!fs::is_directory(path)) fs::create_directories(path);
	files::write_json(path / fs::path(actualName + ".json"), &root);
}

void workshop::saveDocument(std::shared_ptr<xml::Document> document, const std::filesystem::path& packFolder, const std::string& actualName) {
	std::ofstream os;
	os.open(packFolder / "layouts" / (actualName + ".xml"));
	os << xml::stringify(document);
	os.close();
}
