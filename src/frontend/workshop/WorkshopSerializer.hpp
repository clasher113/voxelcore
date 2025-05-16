#pragma once

#include <filesystem>
#include <string>

#include "data/dv.hpp"

class Block;
struct ItemDef;
struct EntityDef;
struct ContentPack;
namespace xml {
	class Document;
}
namespace rigging {
	class Bone;
}

namespace workshop {
	extern std::string stringify(const dv::value& root, bool nice = true);

	extern dv::value toJson(const Block& block, const std::string& actualName, const Block* const parent, const std::string& newParent);
	extern dv::value toJson(const ItemDef& item, const std::string& actualName, const ItemDef* const parent, const std::string& newParent);
	extern dv::value toJson(const EntityDef& entity, const std::string& actualName, const EntityDef* const parent, const std::string& newParent);
	extern dv::value toJson(const rigging::Bone& rootBone, const std::string& actualName);

	extern void saveContentPack(const ContentPack& pack);
	extern void saveBlock(const Block& block, const std::filesystem::path& packFolder, const std::string& actualName, const Block* const parent, const std::string& newParent);
	extern void saveItem(const ItemDef& item, const std::filesystem::path& packFolder, const std::string& actualName, const ItemDef* const parent, const std::string& newParent);
	extern void saveEntity(const EntityDef& entity, const std::filesystem::path& packFolder, const std::string& actualName, const EntityDef* const parent, const std::string& newParent);
	extern void saveSkeleton(const rigging::Bone& root, const std::filesystem::path& packFolder, const std::string& actualName);
	extern void saveDocument(const xml::Document& document, const std::filesystem::path& packFolder, const std::string& actualName);
}