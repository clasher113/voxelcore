#include "WorkshopSerializer.hpp"

#include "coders/json.hpp"
#include "coders/xml.hpp"
#include "content/ContentPack.hpp"
#include "data/dynamic.hpp"
#include "files/files.hpp"
#include "items/ItemDef.hpp"
#include "objects/EntityDef.hpp"
#include "voxels/Block.hpp"
#include "WorkshopUtils.hpp"
#include "constants.hpp"
#include "util/stringutil.hpp"

#include <algorithm>
#include <map>

static void putVec3(dynamic::List& list, const glm::vec3& vector){
	list.put(vector.x); list.put(vector.y); list.put(vector.z);
}

static void putAABB(dynamic::List& list, AABB aabb){
	aabb.b -= aabb.a;
	putVec3(list, aabb.a); putVec3(list, aabb.b);
}

static std::string to_string(const glm::vec3& vector, char delimiter = ','){
	return std::to_string(vector.x) + delimiter + std::to_string(vector.y) + delimiter + std::to_string(vector.z);
}

std::string workshop::stringify(const dynamic::Map_sptr root, bool nice) {
	json::precision = 5;
	std::string result = json::stringify(root, nice, "  ");
	json::precision = 15;

	return result;
}

dynamic::Map_sptr workshop::toJson(const Block& block, const std::string& actualName, const Block* const parent, const std::string& newParent) {
	Block temp(block.name);
	const Block& master = (parent ? *parent : temp);

	dynamic::Map& root = *new dynamic::Map();
	if (block.model != BlockModel::custom) {
		if (!std::equal(block.textureFaces->begin(), block.textureFaces->end(), master.textureFaces->begin(), master.textureFaces->end())) {
			if (std::all_of(std::cbegin(block.textureFaces), std::cend(block.textureFaces), [&r = block.textureFaces[0]](const std::string& value) {return value == r; })) {
				root.put("texture", block.textureFaces[0]);
			}
			else {
				auto& texarr = root.putList("texture-faces");
				for (size_t i = 0; i < 6; i++) {
					texarr.put(block.textureFaces[i]);
				}
			}
		}
	}
#define NOT_EQUAL(MEMBER) master.MEMBER != block.MEMBER
	
	if (!newParent.empty()) root.put("parent", newParent);
	if (NOT_EQUAL(ambientOcclusion)) root.put("ambient-occlusion", block.ambientOcclusion);
	if (NOT_EQUAL(lightPassing)) root.put("light-passing", block.lightPassing);
	if (NOT_EQUAL(skyLightPassing)) root.put("sky-light-passing", block.skyLightPassing);
	if (NOT_EQUAL(obstacle)) root.put("obstacle", block.obstacle);
	if (NOT_EQUAL(selectable)) root.put("selectable", block.selectable);
	if (NOT_EQUAL(replaceable)) root.put("replaceable", block.replaceable);
	if (NOT_EQUAL(breakable)) root.put("breakable", block.breakable);
	if (NOT_EQUAL(grounded)) root.put("grounded", block.grounded);
	if (NOT_EQUAL(drawGroup)) root.put("draw-group", static_cast<int64_t>(block.drawGroup));
	if (NOT_EQUAL(hidden)) root.put("hidden", block.hidden);
	if (NOT_EQUAL(shadeless)) root.put("shadeless", block.shadeless);
	if (NOT_EQUAL(tickInterval)) root.put("tick-interval", block.tickInterval);
	if (NOT_EQUAL(inventorySize)) root.put("inventory-size", static_cast<int64_t>(block.inventorySize));
	if (NOT_EQUAL(model)) root.put("model", to_string(block.model));
	if (NOT_EQUAL(pickingItem)) root.put("picking-item", block.pickingItem);
	if (NOT_EQUAL(scriptName)) root.put("script-name", block.scriptName);
	if (NOT_EQUAL(material)) root.put("material", block.material);
	if (NOT_EQUAL(uiLayout)) root.put("ui-layout", block.uiLayout);
	
	if (!std::equal(std::begin(master.emission), std::end(master.emission), std::begin(block.emission))) {
		auto& emissionarr = root.putList("emission");
		emissionarr.multiline = false;
		for (size_t i = 0; i < 3; i++) {
			emissionarr.put(static_cast<int64_t>(block.emission[i]));
		}
	}

	if (NOT_EQUAL(size)) {
		dynamic::List& sizeList = root.putList("size");
		sizeList.multiline = false;
		putVec3(sizeList, block.size);
	}

	if (!std::equal(block.hitboxes.begin(), block.hitboxes.end(), master.hitboxes.begin(), master.hitboxes.end(), [](const AABB& hitbox, const AABB& modelBox) {
		return hitbox == modelBox;
	})) {
		if (block.hitboxes.size() == 1) {
			const AABB& hitbox = block.hitboxes.front();
			AABB aabb;
			if (block.model == BlockModel::custom || !(aabb == hitbox)) {
				auto& boxarr = root.putList("hitbox");
				boxarr.multiline = false;
				putAABB(boxarr, hitbox);
			}
		}
		else if (!block.hitboxes.empty()) {
			if (!std::equal(block.hitboxes.begin(), block.hitboxes.end(), block.modelBoxes.begin(), block.modelBoxes.end(), [](const AABB& hitbox, const AABB& modelBox) {
				return hitbox == modelBox;
			})) {
				auto& hitboxesArr = root.putList("hitboxes");
				for (const auto& hitbox : block.hitboxes) {
					auto& hitboxArr = hitboxesArr.putList();
					hitboxArr.multiline = false;
					putAABB(hitboxArr, hitbox);
				}
			}
		}
	}

	auto isElementsEqual = [](const std::vector<std::string>& arr, size_t offset, size_t numElements) {
		return std::all_of(std::cbegin(arr) + offset, std::cbegin(arr) + offset + numElements, [&r = arr[offset]](const std::string& value) {return value == r; });
	};
	if (block.model == BlockModel::custom) {
		dynamic::Map& model = root.putMap("model-primitives");
		size_t textureOffset = 0;
		const size_t parentTexSize = parent ? parent->modelTextures.size() : 0;
		if (!block.modelBoxes.empty()) {
			const size_t parentPrimitiveSize = parent ? parent->modelBoxes.size() : 0;
			if (parentPrimitiveSize != block.modelBoxes.size()) {
				dynamic::List& aabbs = model.putList("aabbs");
				for (size_t i = parentPrimitiveSize; i < block.modelBoxes.size(); i++) {
					dynamic::List& aabb = aabbs.putList();
					aabb.multiline = false;
					putAABB(aabb, block.modelBoxes[i]);
					const size_t textures = (isElementsEqual(block.modelTextures, textureOffset, 6) ? 1 : 6);
					for (size_t j = 0; j < textures; j++) {
						aabb.put(block.modelTextures[textureOffset + j - parentTexSize]);
					}
					textureOffset += 6;
				}
			}
		}
		if (!block.modelExtraPoints.empty()) {
			const size_t parentPrimitiveSize = parent ? parent->modelBoxes.size() : 0;
			if (parentPrimitiveSize != block.modelExtraPoints.size()) {
				dynamic::List& tetragons = model.putList("tetragons");
				for (size_t i = parentPrimitiveSize; i < block.modelExtraPoints.size(); i += 4) {
					dynamic::List& tetragon = tetragons.putList();
					tetragon.multiline = false;
					putVec3(tetragon, block.modelExtraPoints[i]);
					putVec3(tetragon, block.modelExtraPoints[i + 1] - block.modelExtraPoints[i]);
					putVec3(tetragon, block.modelExtraPoints[i + 3] - block.modelExtraPoints[i]);
					tetragon.put(block.modelTextures[textureOffset - parentTexSize]);
					textureOffset++;
				}
			}
		}
		if (model.size() == 0) root.remove("model-primitives");
	}

	return std::shared_ptr<dynamic::Map>(&root);
#undef NOT_EQUAL
}

dynamic::Map_sptr workshop::toJson(const ItemDef& item, const std::string& actualName, const ItemDef* const parent, const std::string& newParent) {
	ItemDef temp(item.name);
	const ItemDef& master = (parent ? *parent : temp);

#define NOT_EQUAL(MEMBER) master.MEMBER != item.MEMBER

	dynamic::Map& root = *new dynamic::Map();

	if (!newParent.empty()) root.put("parent", newParent);
	if (NOT_EQUAL(stackSize)) root.put("stack-size", static_cast<int64_t>(item.stackSize));
	if (NOT_EQUAL(iconType)) {
		std::string iconStr("");
		switch (item.iconType) {
			case item_icon_type::none: iconStr = "none";
				break;
			case item_icon_type::block: iconStr = "block";
				break;
			case item_icon_type::sprite: iconStr = "sprite";
				break;
		}
		if (!iconStr.empty()) root.put("icon-type", iconStr);
	}
	if (NOT_EQUAL(icon)) root.put("icon", item.icon);
	if (NOT_EQUAL(placingBlock)) root.put("placing-block", item.placingBlock);
	if (NOT_EQUAL(scriptName)) root.put("script-name", item.scriptName);

	if (!std::equal(std::begin(master.emission), std::end(master.emission), std::begin(item.emission))) {
		auto& emissionarr = root.putList("emission");
		emissionarr.multiline = false;
		for (size_t i = 0; i < 3; i++) {
			emissionarr.put(static_cast<int64_t>(item.emission[i]));
		}
	}

	return std::shared_ptr<dynamic::Map>(&root);
#undef NOT_EQUAL
}

dynamic::Map_sptr workshop::toJson(const EntityDef& entity, const std::string& actualName, const EntityDef* const parent, const std::string& newParent) {
	EntityDef temp(entity.name);
	const EntityDef& master = (parent ? *parent : temp);

#define NOT_EQUAL(MEMBER) master.MEMBER != entity.MEMBER

	dynamic::Map& root = *new dynamic::Map();

	if (!newParent.empty()) root.put("parent", newParent);
	if (NOT_EQUAL(save.enabled)) root.put("save", entity.save.enabled);
	if (NOT_EQUAL(save.skeleton.pose)) root.put("save-skeleton-pose", entity.save.skeleton.pose);
	if (NOT_EQUAL(save.skeleton.textures)) root.put("save-skeleton-textures", entity.save.skeleton.textures);
	if (NOT_EQUAL(save.body.velocity)) root.put("save-body-velocity", entity.save.body.velocity);
	if (NOT_EQUAL(save.body.settings)) root.put("save-body-settings", entity.save.body.settings);
	if (NOT_EQUAL(skeletonName)) root.put("skeleton-name", entity.skeletonName);
	if (NOT_EQUAL(blocking)) root.put("blocking", entity.blocking);
	if (NOT_EQUAL(bodyType)) root.put("body-type", to_string(entity.bodyType));
	if (NOT_EQUAL(hitbox)) {
		dynamic::List& list = root.putList("hitbox");
		list.multiline = false;
		putVec3(list, entity.hitbox);
	}
	if (NOT_EQUAL(components.size())) {
		dynamic::List& list = root.putList("components");
		for (size_t i = parent ? parent->components.size() : 0; i < entity.components.size(); i++) {
			list.put(entity.components[i]);
		}
	}
	std::map<size_t, std::string> sensorsMap;
	if (NOT_EQUAL(boxSensors.size())){
		for (size_t i = parent ? parent->boxSensors.size() : 0; i < entity.boxSensors.size(); i++) {
			const auto& sensor = entity.boxSensors[i];
			sensorsMap.emplace(sensor.first, "aabb," + to_string(sensor.second.a) + ',' + to_string(sensor.second.b));
		}
	}
	if (NOT_EQUAL(radialSensors.size())){
		for (size_t i = parent ? parent->radialSensors.size() : 0; i < entity.radialSensors.size(); i++) {
			const auto& sensor = entity.radialSensors[i];
			sensorsMap.emplace(sensor.first, "radius," + std::to_string(sensor.second));
		}
	}
	if (!sensorsMap.empty()){
		dynamic::List& sensors = root.putList("sensors");
		for (const auto& sensor : sensorsMap){
			auto split = util::split(sensor.second, ',');
			dynamic::List& list = sensors.putList();
			list.multiline = false;
			list.put(split[0]);
			for (size_t i = 1; i < split.size(); i++) {
				list.put(stof(split[i]));
			}
		}
	}

	return std::shared_ptr<dynamic::Map>(&root);
#undef NOT_EQUAL
}

void workshop::saveContentPack(const ContentPack& pack) {
	dynamic::Map root;
	if (!pack.id.empty()) root.put("id", pack.id);
	if (!pack.title.empty()) root.put("title", pack.title);
	if (!pack.version.empty()) root.put("version", pack.version);
	if (!pack.creator.empty()) root.put("creator", pack.creator);
	if (!pack.description.empty()) root.put("description", pack.description);

	if (!pack.dependencies.empty()) {
		auto& dependencies = root.putList("dependencies");
		for (const auto& elem : pack.dependencies) {
			if (!elem.id.empty()) dependencies.put(elem.id);
		}
		if (dependencies.size() == 0) root.remove("dependencies");
	}
	files::write_json(pack.folder / ContentPack::PACKAGE_FILENAME, &root);
}

void workshop::saveBlock(const Block& block, const fs::path& packFolder, const std::string& actualName, const Block* const parent, const std::string& newParent) {
	fs::path path = packFolder / ContentPack::BLOCKS_FOLDER;
	if (!fs::is_directory(path)) fs::create_directories(path);
	files::write_string(path / fs::path(actualName + ".json"), stringify(toJson(block, actualName, parent, newParent), true));
}

void workshop::saveItem(const ItemDef& item, const fs::path& packFolder, const std::string& actualName, const ItemDef* const parent, const std::string& newParent) {
	fs::path path = packFolder / ContentPack::ITEMS_FOLDER;
	if (!fs::is_directory(path)) fs::create_directories(path);
	files::write_string(path / fs::path(actualName + ".json"), stringify(toJson(item, actualName, parent, newParent), true));
}

void workshop::saveEntity(const EntityDef& entity, const std::filesystem::path& packFolder, const std::string& actualName, const EntityDef* const parent, const std::string& newParent) {
	fs::path path = packFolder / ContentPack::ENTITIES_FOLDER;
	if (!fs::is_directory(path)) fs::create_directories(path);
	files::write_string(path / fs::path(actualName + ".json"), stringify(toJson(entity, actualName, parent, newParent), true));
}

void workshop::saveDocument(std::shared_ptr<xml::Document> document, const fs::path& packFolder, const std::string& actualName) {
	std::ofstream os;
	os.open(packFolder / LAYOUTS_FOLDER / (actualName + ".xml"));
	os << xml::stringify(document);
	os.close();
}
