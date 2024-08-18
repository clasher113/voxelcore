#ifndef FRONTEND_MENU_BLOCK_MODEL_CONVERTER_HPP
#define FRONTEND_MENU_BLOCK_MODEL_CONVERTER_HPP

#include <string>
#include <vector>

class Block;
class Atlas;
struct ContentPack;
namespace dynamic{
	class Map;
}
namespace gui{
	class UINode;
}

namespace workshop {
	class BlockModelConverter {
	public:
		static Block* convert(const ContentPack& currentPack, Atlas* blocksAtlas, const dynamic::Map& model, const std::string& blockName,
			const std::vector<std::string>& textureList);
	};
}
#endif // !FRONTEND_MENU_BLOCK_MODEL_CONVERTER_HPP