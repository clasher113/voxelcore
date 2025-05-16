#pragma once

#include "maths/aabb.hpp"
#include "maths/UVRegion.hpp"

#include <filesystem>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>

class Block;
class Atlas;
struct ContentPack;

namespace workshop {
	class BlockModelConverter {
	public:
		BlockModelConverter(const std::filesystem::path& filePath, Atlas* blocksAtlas);

		size_t prepareTextures();
		std::unordered_map<std::string, std::string>& getTextureMap() { return textureList; };

		Block* convert(const ContentPack& currentPack, const std::string& blockName);
	private:
		struct TextureData {
			int rotation = 0;
			UVRegion uv;
			std::string name;
			bool isUnique() const;
		};
		struct PrimitiveData {
			glm::vec3 rotation{ 0.f };
			glm::bvec3 axis{ false };
			glm::vec3 origin{ 0.f };
			AABB aabb;
			TextureData textures[6];
		};

		Atlas* blocksAtlas;
		std::vector<PrimitiveData> primitives;
		std::vector<std::string> preparedTextures;
		std::unordered_map<std::string, std::string> textureList;
		std::unordered_map<std::string, std::vector<TextureData>> croppedTextures;
	};
}