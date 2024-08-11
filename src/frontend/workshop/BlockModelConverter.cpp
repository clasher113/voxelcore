#include "BlockModelConverter.hpp"
#include "WorkshopScreen.hpp"

#include "IncludeCommons.hpp"
#include "WorkshopUtils.hpp"
#include "WorkshopPreview.hpp"
#include "WorkshopSerializer.hpp"
#include "../../graphics/core/Atlas.hpp"
#include "../../voxels/Block.hpp"
#include "../../coders/json.hpp"
#include "../../files/files.hpp"
#include "../../core_defs.hpp"
#include "../../graphics/core/Texture.hpp"
#include "../../coders/imageio.hpp"
#include "../../content/Content.hpp"

#include "libs/portable-file-dialogs.h"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/hash.hpp"

void workshop::WorkShopScreen::createBlockConverterPanel(Block& block, float posX) {
	createPanel([this, &block]() {
		std::shared_ptr<gui::Panel> panel = std::make_shared<gui::Panel>(glm::vec2(200.f));
		panel->setColor(glm::vec4(0.f, 0.f, 0.f, 1.f));

		panel->add(std::make_shared<gui::Label>(L"Import model"));

		auto button = std::make_shared<gui::Button>(L"Choose file", glm::vec4(10.f), gui::onaction());
		button->listenAction([this, button, panel, &block](gui::GUI*) {
			auto file = pfd::open_file("", "", { "(.json)", "*.json" }, pfd::opt::none).result();
			if (file.empty()) return;

			std::string source = files::read_string(file[0]);
			auto model = json::parse(source);

			clearRemoveList(panel);

			panel->add(removeList.emplace_back(std::make_shared<gui::Label>("File: " + fs::path(file[0]).stem().string())));

			panel->add(removeList.emplace_back(std::make_shared<gui::Label>("Texture list")));
			auto textureMap = model->map("textures");
			if (textureMap != nullptr) {
				for (const auto& elem : textureMap->values){
					auto textureButton = std::make_shared<gui::IconButton>(glm::vec2(panel->getSize().x, 50.f), TEXTURE_NOTFOUND, blocksAtlas, TEXTURE_NOTFOUND, elem.first);
					textureButton->listenAction([this, textureButton, panel](gui::GUI*) {
						createTextureList(50.f, 6, DefType::BLOCK, panel->getPos().x + panel->getSize().x, true, [this, textureButton](const std::string& textureName) {
							textureButton->setIcon(getAtlas(assets, textureName), getTexName(textureName));
							textureButton->setText(getTexName(textureName));
							textureButton->setId(getTexName(textureName));
							removePanel(6);
						});
					});
					panel->add(textureButton);
				}
			}

			panel->add(removeList.emplace_back(std::make_shared<gui::Button>(L"Convert", glm::vec4(10.f), [this, panel, button, &block](gui::GUI*) {
				std::vector<std::string> textureList;
				for (const auto& elem : panel->getNodes()){
					auto iconButton = std::dynamic_pointer_cast<gui::IconButton>(elem);
					if (!iconButton) continue;
					const std::string id = iconButton->getId();
					textureList.emplace_back(id.empty() ? TEXTURE_NOTFOUND : id);
				}
				std::string actualName(block.name.substr(currentPack.id.size() + 1));

				if (Block* converted = BlockModelConverter::convert(currentPack, blocksAtlas, button->getId(), actualName, textureList)){
					bool append = false;
					blockid_t id = block.rt.id;
					removePanels(1);
					initialize();
					Block* converting = indices->blocks.get(id);
					if (!append) {
						converting->modelBoxes.clear();
						converting->modelTextures.clear();
						converting->modelExtraPoints.clear();
					}
					converting->modelBoxes = converted->modelBoxes;
					converting->modelTextures = converted->modelTextures;
					converting->modelExtraPoints = converted->modelExtraPoints;
					converting->model = BlockModel::custom;
					preview->updateCache();
					preview->setBlock(converting);

					createContentList(DefType::BLOCK);
					createBlockEditor(*converting);
					createCustomModelEditor(*converting, 0, PrimitiveType::AABB);
				}
			})));

			button->setId(source);
		});
		panel->add(button);

		return panel;
	}, 5, posX);
}

Block* workshop::BlockModelConverter::convert(const ContentPack& currentPack, Atlas* blocksAtlas, const std::string& source, const std::string& blockName,
	const std::vector<std::string>& textureList)
{
	Block* block = nullptr;

	try {
		float gridSize = 16.f;
		Texture* blocksTexture = blocksAtlas->getTexture();
		std::unique_ptr<ImageData> imageData = blocksTexture->readData();
		glm::vec2 textureSize(16.f);
		std::vector<std::string> facesNames{ "east", "west", "down", "up", "south", "north" };
		std::unordered_map<std::string, std::string> textures;
		std::unordered_map<std::string, std::unordered_map<glm::ivec4, std::string>> croppedTextures;

		auto model = json::parse(source);

		auto elementList = model->list("elements");
		if (elementList == nullptr) return false;

		auto textureMap = model->map("textures");
		if (textureMap != nullptr) {
			for (size_t i = 0; i < textureMap->size(); i++) {
				auto it = textureMap->values.begin();
				std::advance(it, i);
				textures.emplace(it->first, textureList[i]);
			}
		};

		for (size_t i = 0; i < elementList->size(); i++) {
			if (block == nullptr) block = new Block("");

			const dynamic::Map_sptr& element = elementList->map(i);

			float angle = 0.f;
			glm::vec3 axis(0.f);
			glm::vec3 origin(0.f);

			PrimitiveType primitiveType = PrimitiveType::AABB;
			if (element->has("rotation")){
				primitiveType = PrimitiveType::TETRAGON;
				const dynamic::Map_sptr& rotationMap = element->map("rotation");
				std::string axis_str;
				std::set<std::string> a{ "x", "y", "z" };
				rotationMap->str("axis", axis_str);
				axis[std::distance(a.begin(), a.find(axis_str))] = 1.f;
				rotationMap->num("angle", angle);
				const dynamic::List_sptr& originList = rotationMap->list("origin");
				origin = glm::vec3(1.f - originList->num(0) / gridSize, originList->num(1) / gridSize, originList->num(2) / gridSize);
			}

			const dynamic::List_sptr& fromList = element->list("from");
			const dynamic::List_sptr& toList = element->list("to");

			glm::vec3 from(fromList->num(0) / gridSize, fromList->num(1) / gridSize, 1 - fromList->num(2) / gridSize);
			glm::vec3 to(toList->num(0) / gridSize, toList->num(1) / gridSize, 1 - toList->num(2) / gridSize);

			if (primitiveType == PrimitiveType::AABB) {
				block->modelBoxes.emplace_back(AABB(from, to));
			}
			else if (primitiveType == PrimitiveType::TETRAGON){
				size_t index = block->modelExtraPoints.size();
				// east (left)
				block->modelExtraPoints.emplace_back(from.x, from.y, to.z);
				block->modelExtraPoints.emplace_back(from.x, from.y, from.z);
				block->modelExtraPoints.emplace_back(from.x, to.y, from.z);
				block->modelExtraPoints.emplace_back(from.x, to.y, to.z);
				
				// west (right)
				block->modelExtraPoints.emplace_back(to.x, from.y, from.z);
				block->modelExtraPoints.emplace_back(to.x, from.y, to.z);
				block->modelExtraPoints.emplace_back(to.x, to.y, to.z);
				block->modelExtraPoints.emplace_back(to.x, to.y, from.z);

				// down (bottom)
				block->modelExtraPoints.emplace_back(to.x, from.y, from.z);
				block->modelExtraPoints.emplace_back(from.x, from.y, from.z);
				block->modelExtraPoints.emplace_back(from.x, from.y, to.z);
				block->modelExtraPoints.emplace_back(to.x, from.y, to.z);

				// up (top)
				block->modelExtraPoints.emplace_back(to.x, to.y, to.z);
				block->modelExtraPoints.emplace_back(from.x, to.y, to.z);
				block->modelExtraPoints.emplace_back(from.x, to.y, from.z);
				block->modelExtraPoints.emplace_back(to.x, to.y, from.z);

				// south (back)
				block->modelExtraPoints.emplace_back(to.x, from.y, to.z);
				block->modelExtraPoints.emplace_back(from.x, from.y, to.z);
				block->modelExtraPoints.emplace_back(from.x, to.y, to.z);
				block->modelExtraPoints.emplace_back(to.x, to.y, to.z);

				// north (front)
				block->modelExtraPoints.emplace_back(from.x, from.y, from.z);
				block->modelExtraPoints.emplace_back(to.x, from.y, from.z);
				block->modelExtraPoints.emplace_back(to.x, to.y, from.z);
				block->modelExtraPoints.emplace_back(from.x, to.y, from.z);

				glm::mat4 mat = glm::translate(glm::mat4(1.f), origin);
				mat = glm::rotate(mat, glm::radians(angle), axis * glm::vec3(-1.f, 1.f,-1.f));
				mat = glm::translate(mat,-origin);

				for (size_t i = index; i < index + 24; i++) {
					block->modelExtraPoints[i] = mat * glm::vec4(block->modelExtraPoints[i], 1.f);;
				}
			}
			const dynamic::Map_sptr& facesMap = element->map("faces");
			for (const std::string& faceName : facesNames){
				const dynamic::Map_sptr& faceMap = facesMap->map(faceName);

				std::string textureName = TEXTURE_NOTFOUND;

				if (faceMap != nullptr) {
					faceMap->str("texture", textureName);
					textureName.erase(0, 1);
					if (textures.find(textureName) != textures.end()) {
						textureName = textures[textureName];
					}
					else textureName = TEXTURE_NOTFOUND;

					const UVRegion& textureUV = blocksAtlas->get(textureName);

					const dynamic::List_sptr& uvList = faceMap->list("uv");
					UVRegion faceUV(uvList->num(0) / textureSize.x, uvList->num(1) / textureSize.y,
									uvList->num(2) / textureSize.x, uvList->num(3) / textureSize.y);

					glm::ivec2 position = glm::ivec2(textureUV.u1 * blocksTexture->getWidth(), textureUV.v1 * blocksTexture->getHeight());
					glm::ivec2 size = glm::ivec2(textureUV.u2 * blocksTexture->getWidth(), textureUV.v2 * blocksTexture->getHeight()) - position;

					glm::ivec2 localPos = glm::ivec2(faceUV.u1 * size.x, faceUV.v1 * size.y);
					position += localPos;
					size = glm::ivec2(faceUV.u2 * size.x, faceUV.v2 * size.y) - localPos;

					glm::ivec4 rect(localPos, size);

					{
						auto textureParts = croppedTextures.find(textureName);
						if (textureParts != croppedTextures.end()){
							if (textureParts->second.find(rect) != textureParts->second.end()){
								textureName = textureParts->second[rect];
							}
							else{
								textureName = blockName + "_" + textureName + "_" + std::to_string(textureParts->second.size());
								textureParts->second.emplace(rect, textureName);
							}
						}
						else {
							auto pair = croppedTextures.emplace(textureName, std::unordered_map<glm::ivec4, std::string>());
							textureName = blockName + "_" + textureName + "_" + std::to_string(pair.first->second.size());
							pair.first->second.emplace(rect, textureName);
						}
					}
					
					bool flipX = false, flipY = false;

					if (size.x < 0){
						size.x = abs(size.x);
						position.x -= size.x;
						flipX = true;
					}
					if (size.y < 0) {
						size.y = abs(size.y);
						position.y = size.y;
						flipY = true;
					}

					ubyte* data = new ubyte[size.x * size.y * 4];

					unsigned int location = (position.y + size.y - 1)* blocksTexture->getWidth() * 4 + position.x * 4;

					for (int j = 0; j < size.y; j++) {
						memcpy(&data[j * (size.x * 4)], &imageData->getData()[location], size.x * 4);
						location -= blocksTexture->getWidth() * 4;
					}

					ImageData image(ImageFormat::rgba8888, size.x, size.y, data);
					if (flipX) image.flipX();
					if (flipY) image.flipY();

					imageio::write(fs::path(currentPack.folder / TEXTURES_FOLDER / ContentPack::BLOCKS_FOLDER / (textureName + ".png")).string(), &image);
					delete[] data;
				}
				block->modelTextures.emplace_back(textureName);
			}
		}
	}
	catch (const std::exception&) {
		return block;
	}
	return block;
}