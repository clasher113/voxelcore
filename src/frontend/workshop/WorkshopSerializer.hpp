#ifndef FRONTEND_MENU_WORKSHOP_SERIALIZER_HPP
#define FRONTEND_MENU_WORKSHOP_SERIALIZER_HPP

#include <filesystem>
#include <string>

class Block;
struct ItemDef;
struct ContentPack;
namespace xml {
	class Document;
}

namespace workshop {
	extern void saveContentPack(const ContentPack& pack);
	extern void saveBlock(const Block& block, const std::filesystem::path& packFolder, const std::string& actualName);
	extern void saveItem(const ItemDef& item, const std::filesystem::path& packFolder, const std::string& actualName);
	extern void saveDocument(std::shared_ptr<xml::Document> document, const std::filesystem::path& packFolder, const std::string& actualName);
}

#endif // !FRONTEND_MENU_WORKSHOP_SERIALIZER_HPP
