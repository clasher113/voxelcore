#ifndef FRONTEND_MENU_WORKSHOP_SERIALIZER_HPP
#define FRONTEND_MENU_WORKSHOP_SERIALIZER_HPP

#include <filesystem>
#include <string>

#include "data/dynamic_fwd.hpp"

class Block;
struct ItemDef;
struct EntityDef;
struct ContentPack;
namespace xml {
	class Document;
}

namespace workshop {
	extern std::string stringify(const dynamic::Map_sptr root, bool nice = true);

	extern dynamic::Map_sptr toJson(const Block& block, const std::string& actualName, const Block* const parent, const std::string& newParent);
	extern dynamic::Map_sptr toJson(const ItemDef& item, const std::string& actualName, const ItemDef* const parent, const std::string& newParent);
	extern dynamic::Map_sptr toJson(const EntityDef& entity, const std::string& actualName, const EntityDef* const parent, const std::string& newParent);

	extern void saveContentPack(const ContentPack& pack);
	extern void saveBlock(const Block& block, const std::filesystem::path& packFolder, const std::string& actualName, const Block* const parent, const std::string& newParent);
	extern void saveItem(const ItemDef& item, const std::filesystem::path& packFolder, const std::string& actualName, const ItemDef* const parent, const std::string& newParent);
	extern void saveEntity(const EntityDef& entity, const std::filesystem::path& packFolder, const std::string& actualName, const EntityDef* const parent, const std::string& newParent);
	extern void saveDocument(std::shared_ptr<xml::Document> document, const std::filesystem::path& packFolder, const std::string& actualName);
}

#endif // !FRONTEND_MENU_WORKSHOP_SERIALIZER_HPP
