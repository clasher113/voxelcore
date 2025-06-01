#pragma once

#include <filesystem>
#include <string>

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
	extern std::string stringify(const Block& block, const Block* const currentParent = nullptr, const std::string& newParent = "");
	extern std::string stringify(const ItemDef& item, const ItemDef* const currentParent = nullptr, const std::string& newParent = "");
	extern std::string stringify(const EntityDef& entity, const EntityDef* const currentParent = nullptr, const std::string& newParent = "");
	extern std::string stringify(const rigging::Bone& rootBone);

	extern void saveContentPack(const ContentPack& pack);
	extern void saveBlock(const Block& block, const std::filesystem::path& packFolder, const Block* const currentParent = nullptr,
		const std::string& newParent = "");
	extern void saveItem(const ItemDef& item, const std::filesystem::path& packFolder, const ItemDef* const currentParent = nullptr,
		const std::string& newParent = "");
	extern void saveEntity(const EntityDef& entity, const std::filesystem::path& packFolder, const EntityDef* const currentParent = nullptr,
		const std::string& newParent = "");
	extern void saveSkeleton(const rigging::Bone& root, const std::filesystem::path& packFolder, const std::string& actualName);
	extern void saveDocument(const xml::Document& document, const std::filesystem::path& packFolder, const std::string& actualName);
}