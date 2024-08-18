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

#define NOMINMAX
#include "libs/portable-file-dialogs.h"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/hash.hpp"

struct CroppedTextureData {
	int rotation;
	glm::ivec4 rect;
	std::string name;
};

static void rotateImage(std::unique_ptr<ImageData>& image, int angleDegrees);

void workshop::WorkShopScreen::createBlockConverterPanel(Block& block, float posX) {
	createPanel([this, &block]() {
		std::shared_ptr<gui::Panel> panel = std::make_shared<gui::Panel>(glm::vec2(300.f));

		panel->add(std::make_shared<gui::Label>(L"Import model"));

		auto button = std::make_shared<gui::Button>(L"Choose file", glm::vec4(10.f), gui::onaction());
		button->listenAction([this, button, panel, &block](gui::GUI*) {
			if (checkUnsaved()) return;
			auto file = pfd::open_file("", "", { "(.json)", "*.json" }, pfd::opt::none).result();
			if (file.empty()) return;

			std::string source = files::read_string(file[0]);
			auto model = json::parse(source);

			clearRemoveList(panel);

			std::vector<std::shared_ptr<gui::UINode>> nodes;

			nodes.emplace_back(std::make_shared<gui::Label>("File: " + fs::path(file[0]).stem().string()));

			nodes.emplace_back(std::make_shared<gui::Label>("Texture list"));
			auto textureMap = model->map("textures");
			if (textureMap != nullptr) {
				for (const auto& elem : textureMap->values) {
					auto textureButton = std::make_shared<gui::IconButton>(glm::vec2(50.f), TEXTURE_NOTFOUND, blocksAtlas, TEXTURE_NOTFOUND, elem.first);
					textureButton->listenAction([this, textureButton, panel](gui::GUI*) {
						createTextureList(50.f, 6, DefType::BLOCK, panel->getPos().x + panel->getSize().x, true, [this, textureButton](const std::string& textureName) {
							textureButton->setIcon(getAtlas(assets, textureName), getTexName(textureName));
							textureButton->setText(getTexName(textureName));
							textureButton->setId(getTexName(textureName));
							removePanel(6);
						});
					});
					nodes.emplace_back(textureButton);
				}
			}

			std::string actualName(block.name.substr(currentPack.id.size() + 1));
			size_t deleteTextures = 0;
			size_t createTextures = 0;
			std::unordered_map<std::string, std::vector<std::pair<glm::vec4, int>>> croppedTextures;

			auto elementList = model->list("elements");
			if (elementList != nullptr) {
				for (size_t i = 0; i < elementList->size(); i++) {
					const dynamic::Map_sptr& faces = elementList->map(i)->map("faces");
					for (size_t j = 0; j < faces->size(); j++) {
						auto it = faces->values.begin();
						std::advance(it, j);
						const dynamic::Map_sptr& faceMap = std::get<dynamic::Map_sptr>(it->second);
						if (faceMap != nullptr) {
							const dynamic::List_sptr& uvList = faceMap->list("uv");
							glm::vec4 faceUV(uvList->num(0), uvList->num(1), uvList->num(2), uvList->num(3));
							if (faceUV.x != 0.f || faceUV.y != 0.f || faceUV.z != 16.f || faceUV.w != 16.f) {
								std::string textureName = faceMap->get<std::string>("texture");
								int rotation = 0;
								if (faceMap->has("rotation")) faceMap->num("rotation", rotation);
								auto textureParts = croppedTextures.find(textureName);
								if (textureParts != croppedTextures.end()) {
									std::vector<std::pair<glm::vec4, int>>& current = textureParts->second;
									auto found = std::find_if(current.begin(), current.end(), [faceUV, rotation](const std::pair<glm::vec4, int>& data) {
										if (data.first == faceUV && data.second == rotation) return true;
										return false;
									});
									if (found == current.end()){
										current.emplace_back(std::pair<glm::vec4, int>(faceUV, rotation));
									}
								}
								else {
									auto pair = croppedTextures.emplace(textureName, std::vector<std::pair<glm::vec4, int>>());
									pair.first->second.emplace_back(std::pair<glm::vec4, int>(faceUV, rotation));
								}
							}
						}
					}
				}
			}
			for (const auto& elem : croppedTextures){
				for (const auto& elem2 : elem.second){
					createTextures++;
				}
			}

			fs::path texturesPath(currentPack.folder / TEXTURES_FOLDER / ContentPack::BLOCKS_FOLDER);
			if (fs::is_directory(texturesPath)) {
				for (const auto& entry : fs::directory_iterator(texturesPath)) {
					if (entry.is_regular_file() && entry.path().extension() == ".png" && entry.path().stem().string().find(actualName + '_') == 0) {
						deleteTextures++;
					}
				}
			}

			auto label = std::make_shared<gui::Label>(std::to_string(deleteTextures) + " Texture(s) will be deleted");
			label->setColor(glm::vec4(1.f, 0.f, 0.f, 1.f));
			nodes.emplace_back(label);
			label = std::make_shared<gui::Label>(std::to_string(createTextures) + " Texture(s) will be created");
			label->setColor(glm::vec4(0.f, 1.f, 0.f, 1.f));
			nodes.emplace_back(label);

			for (const auto& elem : nodes){
				panel->add(removeList.emplace_back(elem));
			}

			panel->add(removeList.emplace_back(std::make_shared<gui::Button>(L"Convert", glm::vec4(10.f), [this, panel, actualName, button, model, &block](gui::GUI*) {
				std::vector<std::string> textureList;
				for (const auto& elem : panel->getNodes()){
					auto iconButton = std::dynamic_pointer_cast<gui::IconButton>(elem);
					if (!iconButton) continue;
					const std::string id = iconButton->getId();
					textureList.emplace_back(id.empty() ? TEXTURE_NOTFOUND : id);
				}

				if (Block* converted = BlockModelConverter::convert(currentPack, blocksAtlas, *model, actualName, textureList)){
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

Block* workshop::BlockModelConverter::convert(const ContentPack& currentPack, Atlas* blocksAtlas, const dynamic::Map& model, const std::string& blockName,
	const std::vector<std::string>& textureList)
{
	Block* block = nullptr;

	try {
		float gridSize = 16.f;
		Texture* blocksTexture = blocksAtlas->getTexture();
		const fs::path texturesPath(currentPack.folder / TEXTURES_FOLDER / ContentPack::BLOCKS_FOLDER);
		const std::unique_ptr<ImageData> imageData = blocksTexture->readData();
		glm::ivec2 textureSize(16.f);
		const std::string facesNames[] = { "east", "west", "down", "up", "south", "north" };
		std::unordered_map<std::string, std::string> textures;
		std::unordered_map<std::string, std::vector<CroppedTextureData>> croppedTextures;

		auto elementList = model.list("elements");
		if (elementList == nullptr) return block;

		//auto textureSizeList = model->list("texture_size");
		//if (textureSizeList != nullptr){
		//	textureSize = glm::vec2(textureSizeList->integer(0), textureSizeList->integer(1));
		//}

		auto textureMap = model.map("textures");
		if (textureMap != nullptr) {
			for (size_t i = 0; i < textureMap->size(); i++) {
				auto it = textureMap->values.begin();
				std::advance(it, i);
				textures.emplace(it->first, textureList[i]);
			}
		};

		{ // delete old textures
			std::string searchString(blockName + '_');
			for (const auto& elem : fs::directory_iterator(texturesPath)) {
				if (elem.is_regular_file() && elem.path().extension() == ".png" && elem.path().stem().string().find(searchString) == 0){
					fs::remove(elem);
				}
			}
		}

		for (size_t i = 0; i < elementList->size(); i++) {
			if (block == nullptr) block = new Block("");

			const dynamic::Map_sptr& element = elementList->map(i);

			float angle = 0.f;
			glm::vec3 axis(0.f);
			glm::vec3 origin(0.f);

			PrimitiveType primitiveType = PrimitiveType::AABB;
			if (element->has("rotation")){
				const dynamic::Map_sptr& rotationMap = element->map("rotation");
				std::string axis_str;
				std::set<std::string> a{ "x", "y", "z" };
				rotationMap->str("axis", axis_str);
				axis[std::distance(a.begin(), a.find(axis_str))] = 1.f;
				rotationMap->num("angle", angle);
				if (angle) primitiveType = PrimitiveType::TETRAGON;
				const dynamic::List_sptr& originList = rotationMap->list("origin");
				origin = glm::vec3(1.f - originList->num(0) / gridSize, originList->num(1) / gridSize, originList->num(2) / gridSize);
			}

			const dynamic::List_sptr& fromList = element->list("from");
			const dynamic::List_sptr& toList = element->list("to");

			glm::vec3 from(1 - fromList->num(0) / gridSize, fromList->num(1) / gridSize, 1 - fromList->num(2) / gridSize);
			glm::vec3 to(1 - toList->num(0) / gridSize, toList->num(1) / gridSize, 1 - toList->num(2) / gridSize);

			if (primitiveType == PrimitiveType::AABB) {
				block->modelBoxes.emplace_back(AABB(from, to));
			}
			else if (primitiveType == PrimitiveType::TETRAGON){
				size_t index = block->modelExtraPoints.size();
				// east (left)
				block->modelExtraPoints.emplace_back(to.x, from.y, to.z);
				block->modelExtraPoints.emplace_back(to.x, from.y, from.z);
				block->modelExtraPoints.emplace_back(to.x, to.y, from.z);
				block->modelExtraPoints.emplace_back(to.x, to.y, to.z);

				// west (right)
				block->modelExtraPoints.emplace_back(from.x, from.y, from.z);
				block->modelExtraPoints.emplace_back(from.x, from.y, to.z);
				block->modelExtraPoints.emplace_back(from.x, to.y, to.z);
				block->modelExtraPoints.emplace_back(from.x, to.y, from.z);

				// down (bottom)
				block->modelExtraPoints.emplace_back(from.x, from.y, from.z);
				block->modelExtraPoints.emplace_back(to.x, from.y, from.z);
				block->modelExtraPoints.emplace_back(to.x, from.y, to.z);
				block->modelExtraPoints.emplace_back(from.x, from.y, to.z);

				// up (top)
				block->modelExtraPoints.emplace_back(from.x, to.y, to.z);
				block->modelExtraPoints.emplace_back(to.x, to.y, to.z);
				block->modelExtraPoints.emplace_back(to.x, to.y, from.z);
				block->modelExtraPoints.emplace_back(from.x, to.y, from.z);

				// south (back)
				block->modelExtraPoints.emplace_back(from.x, from.y, to.z);
				block->modelExtraPoints.emplace_back(to.x, from.y, to.z);
				block->modelExtraPoints.emplace_back(to.x, to.y, to.z);
				block->modelExtraPoints.emplace_back(from.x, to.y, to.z);

				// north (front)
				block->modelExtraPoints.emplace_back(to.x, from.y, from.z);
				block->modelExtraPoints.emplace_back(from.x, from.y, from.z);
				block->modelExtraPoints.emplace_back(from.x, to.y, from.z);
				block->modelExtraPoints.emplace_back(to.x, to.y, from.z);

				glm::mat4 mat = glm::translate(glm::mat4(1.f), origin);
				mat = glm::rotate(mat, glm::radians(angle), axis * glm::vec3(-1.f, 1.f,-1.f));
				mat = glm::translate(mat,-origin);

				for (size_t i = index; i < index + 24; i++) {
					block->modelExtraPoints[i] = mat * glm::vec4(block->modelExtraPoints[i], 1.f);
				}
			}

			std::vector<std::string> facesOrder;

			size_t texturesIndex = block->modelTextures.size();
			block->modelTextures.resize(texturesIndex + 6);

			const dynamic::Map_sptr& facesMap = element->map("faces");
			for (const auto& elem : facesMap->values){
				const dynamic::Map_sptr& faceMap = std::get<dynamic::Map_sptr>(elem.second);
				facesOrder.emplace_back(elem.first);

				size_t currentIndex = texturesIndex + std::distance(std::begin(facesNames), std::find(std::begin(facesNames), std::end(facesNames), elem.first));

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
					UVRegion faceUV(uvList->num(0) / textureSize.x, 1 - uvList->num(1) / textureSize.y,
									uvList->num(2) / textureSize.x, 1 - uvList->num(3) / textureSize.y);

					int rotation = 0;
					if (faceMap->has("rotation")){
						faceMap->num("rotation", rotation);
					}
					if (primitiveType == PrimitiveType::AABB && elem.first == facesNames[3]) {
						rotation += 180;
					}
					glm::ivec2 position = glm::ivec2(textureUV.u1 * blocksTexture->getWidth(), textureUV.v1 * blocksTexture->getHeight());
					glm::ivec2 size = glm::ivec2(textureUV.u2 * blocksTexture->getWidth(), textureUV.v2 * blocksTexture->getHeight()) - position;

					glm::ivec2 localPos = glm::ivec2(faceUV.u1 * size.x, faceUV.v1 * size.y);
					position += localPos;
					size = glm::ivec2(faceUV.u2 * size.x, faceUV.v2 * size.y) - localPos;

					glm::ivec4 rect(localPos, size);

					if (faceUV.u1 != 0.f || faceUV.v1 != 0.f || faceUV.u2 != 1.f || faceUV.v2 != 1.f) {
						auto textureParts = croppedTextures.find(textureName);
						if (textureParts != croppedTextures.end()) {
							std::vector<CroppedTextureData>& current = textureParts->second;
							auto found = std::find_if(current.begin(), current.end(), [rect, rotation](const CroppedTextureData& data) {
								if (data.rect == rect && data.rotation == rotation) return true;
								return false;
							});
							if (found != current.end()) {
								block->modelTextures[currentIndex] = found->name;
								continue;
							}
							else {
								textureName = blockName + "_" + textureName + "_" + std::to_string(textureParts->second.size());
								current.emplace_back(CroppedTextureData{ rotation, rect, textureName });
							}
						}
						else {
							auto pair = croppedTextures.emplace(textureName, std::vector<CroppedTextureData>());
							textureName = blockName + "_" + textureName + "_" + std::to_string(pair.first->second.size());
							pair.first->second.emplace_back(CroppedTextureData{ rotation, rect, textureName });
						}

						bool flipX = false, flipY = false;

						if (size.x < 0) {
							size.x *= -1;
							position.x -= size.x;
							flipX = true;
						}
						if (size.y < 0) {
							size.y *= -1;
							position.y -= size.y;
							flipY = true;
						}
						size = glm::max(size, glm::ivec2(1));

						std::unique_ptr<ubyte[]> data(new ubyte[size.x * size.y * 4]);

						unsigned int location = position.y * blocksTexture->getWidth() * 4 + position.x * 4;

						for (int j = 0; j < size.y; j++) {
							memcpy(&data[j * (size.x * 4)], &imageData->getData()[location], size.x * 4);
							location += blocksTexture->getWidth() * 4;
						}

						std::unique_ptr<ImageData> image(new ImageData(ImageFormat::rgba8888, size.x, size.y, std::move(data)));


						if (flipX) image->flipX();
						if (flipY) image->flipY();
						if (rotation >= 360) rotation -= 360;
						if (rotation){
							rotateImage(image, rotation);
						}

						if (!fs::is_directory(texturesPath)) fs::create_directories(texturesPath);
						imageio::write(fs::path(texturesPath / (textureName + ".png")).string(), image.get());
					}
				}
				block->modelTextures[currentIndex] = textureName;
			}

			if (!facesOrder.empty()) {
				auto& lastFace = facesOrder.begin();

				for (size_t i = 0; i < std::size(facesNames); i++) {
					auto it = std::find(facesOrder.begin(), facesOrder.end(), facesNames[i]);
					if (it != facesOrder.end()) {
						lastFace = it;
					}
					else {
						block->modelTextures[texturesIndex + i] = block->modelTextures[texturesIndex + std::distance(std::begin(facesNames),
							std::find(std::begin(facesNames), std::end(facesNames), *lastFace))];
					}
				}
			}
		}
	}
	catch (const std::exception&) {
		return block;
	}
	return block;
}

static void rotateImage(std::unique_ptr<ImageData>& image, int angleDegrees) {	
	if (image->getFormat() != ImageFormat::rgba8888) return;

	uint width = image->getWidth();
	uint height = image->getHeight();

	int length = width * height * 4;

	uint32_t* original = reinterpret_cast<uint32_t*>(image->getData());
	uint32_t* copy = new uint32_t[length];
	memcpy(copy, original, length);

	if (angleDegrees == 180){
		std::reverse_copy(copy, copy + width * height, original);
	}
	else if (angleDegrees == 90 || angleDegrees == 270) {

		if (width != height) {
			image.reset(new ImageData(ImageFormat::rgba8888, height, width, new ubyte[length]));
			original = reinterpret_cast<uint32_t*>(image->getData());
		}
		bool clockWise = angleDegrees == 90;

		//for (size_t x = 0; x < width; x++) {
		//	for (size_t y = 0; y < height; y++) {
		//		original[x * width + y] = copy[(clockWise ? height - y - 1 : y) * width + x];
		//	}
		//}
		for (size_t y = 0; y < height; y++) {
			for (size_t x = 0; x < width; x++) {
				original[(x + 1) * height - y - 1] = copy[clockWise ?  y * width + x : width * height - 1 - (y * width + x)];
			}
		}
	}
	delete[] copy;
}