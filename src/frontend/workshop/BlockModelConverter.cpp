#include "BlockModelConverter.hpp"
#include "WorkshopScreen.hpp"

#include "../../coders/imageio.hpp"
#include "../../coders/json.hpp"
#include "../../content/Content.hpp"
#include "../../core_defs.hpp"
#include "../../debug/Logger.hpp"
#include "../../files/files.hpp"
#include "../../graphics/core/Atlas.hpp"
#include "../../graphics/core/Texture.hpp"
#include "../../voxels/Block.hpp"
#include "IncludeCommons.hpp"
#include "WorkshopPreview.hpp"
#include "WorkshopSerializer.hpp"
#include "WorkshopUtils.hpp"

#define NOMINMAX
#include "libs/portable-file-dialogs.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <array>

static debug::Logger logger("workshop-converter");

static void rotateImage(std::unique_ptr<ImageData>& image, size_t angleDegrees);

void workshop::WorkShopScreen::createBlockConverterPanel(Block& block, float posX) {
	createPanel([this, &block]() {
		gui::Panel& panel = *new gui::Panel(glm::vec2(300.f));

		std::shared_ptr<BlockModelConverter> converter = nullptr;

		panel += new gui::Label(L"Import model");

		gui::Button& button = *new gui::Button(L"Choose file", glm::vec4(10.f), gui::onaction());
		button.listenAction([this, &button, &panel, converter, &block](gui::GUI*) mutable {
			showUnsaved([=, &block, &panel]() mutable {
				auto file = pfd::open_file("", "", { "(.json)", "*.json" }, pfd::opt::none).result();
				if (file.empty()) return;

				std::string actualName(getDefName(block.name));
				converter = std::make_shared<BlockModelConverter>(file.front(), blocksAtlas);
				const size_t newTextures = converter->prepareTextures();
				size_t deleteTextures = 0;

				fs::path texturesPath(currentPack.folder / TEXTURES_FOLDER / ContentPack::BLOCKS_FOLDER);
				if (fs::is_directory(texturesPath)) {
					for (const auto& entry : fs::directory_iterator(texturesPath)) {
						if (entry.is_regular_file() && entry.path().extension() == ".png" && entry.path().stem().string().find(actualName + '_') == 0) {
							deleteTextures++;
						}
					}
				}

				clearRemoveList(panel);

				std::vector<gui::UINode*> nodes;
				gui::Label& createTexturesLabel = *new gui::Label(std::to_wstring(newTextures) + L" Texture(s) will be created");
				gui::Label& deleteTexturesLabel = *new gui::Label(std::to_wstring(deleteTextures) + L" Texture(s) will be deleted");

				nodes.emplace_back(new gui::Label("File: " + fs::path(file[0]).stem().string()));

				nodes.emplace_back(new gui::Label("Texture list"));
				auto& textureMap = converter->getTextureMap();
				for (auto& pair : textureMap) {
					gui::IconButton& textureButton = *new gui::IconButton(glm::vec2(50.f), pair.second, blocksAtlas, pair.second, pair.first);
					textureButton.listenAction([this, &textureButton, &panel, converter, &createTexturesLabel, &pair](gui::GUI*) {
						createTextureList(50.f, 6, DefType::BLOCK, panel.getPos().x + panel.getSize().x, true, [this, &textureButton, converter, &createTexturesLabel, &pair](const std::string& textureName) {
							pair.second = getTexName(textureName);
							textureButton.setIcon(getAtlas(assets, textureName), pair.second);
							textureButton.setText(pair.second);
							createTexturesLabel.setText(std::to_wstring(converter->prepareTextures()) + L" Texture(s) will be created");
							removePanel(6);
						});
					});
					nodes.emplace_back(&textureButton);
				}

				deleteTexturesLabel.setColor(glm::vec4(1.f, 0.f, 0.f, 1.f));
				nodes.emplace_back(&deleteTexturesLabel);
				createTexturesLabel.setColor(glm::vec4(0.f, 1.f, 0.f, 1.f));
				nodes.emplace_back(&createTexturesLabel);

				nodes.emplace_back(new gui::Button(L"Convert", glm::vec4(10.f), [this, &panel, actualName, converter, &block](gui::GUI*) {
					if (Block* converted = converter->convert(currentPack, actualName)) {
						bool append = false;
						const blockid_t id = block.rt.id;
						removePanels(1);
						initialize();
						Block* converting = indices->blocks.getIterable().at(id);
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
						delete converted;
					}
				}));

				for (gui::UINode* node : nodes) {
					panel += removeList.emplace_back(node);
				}
			});
		});
		panel += button;

		return std::ref(panel);
	}, 5, posX);
}

workshop::BlockModelConverter::BlockModelConverter(const std::filesystem::path& filePath, Atlas* blocksAtlas) :
	blocksAtlas(blocksAtlas)
{
	float gridSize = 16.f;
	glm::ivec2 textureSize(16);

	std::string source = files::read_string(filePath);
	auto model = json::parse(source);

	auto textureMap = model->map("textures");
	if (textureMap != nullptr) {
		for (size_t i = 0; i < textureMap->size(); i++) {
			auto it = textureMap->values.begin();
			std::advance(it, i);
			const std::string textureName = getTexName(std::get<std::string>(it->second), "/");
			textureList.emplace(it->first, blocksAtlas && blocksAtlas->has(textureName) ? textureName : TEXTURE_NOTFOUND);
		}
	};

	auto elementList = model->list("elements");
	if (elementList != nullptr) {
		for (size_t i = 0; i < elementList->size(); i++) {
			const dynamic::Map_sptr& element = elementList->map(i);

			PrimitiveData primitiveData;

			const dynamic::List_sptr& fromList = element->list("from");
			const dynamic::List_sptr& toList = element->list("to");

			primitiveData.aabb.a = glm::vec3(1 - fromList->num(0) / gridSize, fromList->num(1) / gridSize, 1 - fromList->num(2) / gridSize);
			primitiveData.aabb.b = glm::vec3(1 - toList->num(0) / gridSize, toList->num(1) / gridSize, 1 - toList->num(2) / gridSize);

			if (element->has("rotation")) {
				const dynamic::Map_sptr& rotationMap = element->map("rotation");
				size_t axisIndex = 0;
				float angle = 0;

				if (rotationMap->has("axis")) {
					const std::string a[] = { "x", "y", "z" };
					const std::string axisStr = rotationMap->get<std::string>("axis");
					auto it = std::find(std::begin(a), std::end(a), axisStr);
					if (it != std::end(a)) {
						axisIndex = std::distance(std::begin(a), it);
					}
				}
				primitiveData.axis[axisIndex] = true;

				if (rotationMap->has("angle")) {
					angle = rotationMap->get<float>("angle");
					primitiveData.rotation[axisIndex] = angle;
				}

				const dynamic::List_sptr& originList = rotationMap->list("origin");
				if (originList != nullptr)
					primitiveData.origin = glm::vec3(1.f - originList->num(0) / gridSize, originList->num(1) / gridSize, originList->num(2) / gridSize);

				if (rotationMap->has("rescale") && rotationMap->get<bool>("rescale")) {
					glm::vec3 scale(1.f);
					for (glm::length_t i = 0; i < glm::vec3::length(); i++) {
						if (i == axisIndex) continue;
						scale[i] = 1.0F / (angle == 22.5f ? cos(0.39269909262657166) : cos((M_PI / 4.0)));
					}

					glm::mat4 mat = glm::translate(glm::mat4(1.f), glm::vec3(0.5f));
					mat = glm::scale(mat, scale);
					mat = glm::translate(mat, -glm::vec3(0.5f));

					primitiveData.aabb.a = mat * glm::vec4(primitiveData.aabb.a, 1.f);
					primitiveData.aabb.b = mat * glm::vec4(primitiveData.aabb.b, 1.f);
				}
			}

			const dynamic::Map_sptr& facesMap = element->map("faces");
			if (facesMap != nullptr) {
				std::vector<std::string> facesOrder;
				const std::string facesNames[] = { "east", "west", "down", "up", "south", "north" };

				for (const auto& elem : facesMap->values) {
					size_t faceIndex = std::distance(std::begin(facesNames), std::find(std::begin(facesNames), std::end(facesNames), elem.first));
					TextureData& currentTexture = primitiveData.textures[faceIndex];
					const dynamic::Map_sptr& faceMap = std::get<dynamic::Map_sptr>(elem.second);
					facesOrder.emplace_back(elem.first);

					const dynamic::List_sptr& uvList = faceMap->list("uv");
					if (uvList != nullptr) {
						currentTexture.uv = UVRegion(uvList->num(0) / textureSize.x, 1 - uvList->num(1) / textureSize.y,
							uvList->num(2) / textureSize.x, 1 - uvList->num(3) / textureSize.y);
					}
					if (faceMap->has("rotation")) {
						currentTexture.rotation = faceMap->get<int>("rotation");
					}
					if (primitiveData.rotation == glm::vec3(0.f) && elem.first == facesNames[3])
						currentTexture.rotation += 180;
					if (faceMap->has("texture")) {
						currentTexture.name = faceMap->get<std::string>("texture").erase(0, 1);
					}
				}

				if (!facesOrder.empty()) {
					auto lastFace = facesOrder.begin();

					for (size_t j = 0; j < std::size(facesNames); j++) {
						auto it = std::find(facesOrder.begin(), facesOrder.end(), facesNames[j]);
						if (it != facesOrder.end()) {
							lastFace = it;
						}
						else {
							primitiveData.textures[j] = primitiveData.textures[std::distance(std::begin(facesNames),
								std::find(std::begin(facesNames), std::end(facesNames), *lastFace))];
						}
					}
				}
			}

			primitives.emplace_back(primitiveData);
		}
	};
}

size_t workshop::BlockModelConverter::prepareTextures() {
	croppedTextures.clear();
	preparedTextures.clear();
	size_t uniqueTextures = 0;

	for (const auto& primitive : primitives) {
		for (size_t i = 0; i < std::size(primitive.textures); i++) {
			const TextureData& currentTexture = primitive.textures[i];

			std::string textureName = TEXTURE_NOTFOUND;
			if (textureList.find(currentTexture.name) != textureList.end()) {
				textureName = textureList.at(currentTexture.name);
			}

			if (currentTexture.isUnique()) {
				const auto textureParts = croppedTextures.find(textureName);
				if (textureParts != croppedTextures.end()) {
					std::vector<TextureData>& current = textureParts->second;
					auto found = std::find_if(current.begin(), current.end(), [&currentTexture](const TextureData& data) {
						if (data.uv == currentTexture.uv && data.rotation == currentTexture.rotation) return true;
						return false;
					});
					if (found != current.end()) {
						textureName = found->name;
					}
					else {
						textureName.append("_" + std::to_string(textureParts->second.size()));
						current.emplace_back(currentTexture).name = textureName;
						uniqueTextures++;
					}
				}
				else {
					auto pair = croppedTextures.emplace(textureName, std::vector<TextureData>());
					textureName.append("_0");
					pair.first->second.emplace_back(currentTexture).name = textureName;
					uniqueTextures++;
				}
			}

			preparedTextures.emplace_back(textureName);
		}
	}
	return uniqueTextures;
}

Block* workshop::BlockModelConverter::convert(const ContentPack& currentPack, const std::string& blockName) {
	Block* block = nullptr;

	try {
		Texture* const blocksTexture = blocksAtlas->getTexture();
		const fs::path texturesPath(currentPack.folder / TEXTURES_FOLDER / ContentPack::BLOCKS_FOLDER);
		const std::unique_ptr<ImageData> imageData = blocksTexture->readData();

		if (primitives.empty()) return block;
		block = new Block("");
		for (const auto& primitive : primitives) { // apply new primitives and textures
			const AABB aabb(primitive.aabb.min(), primitive.aabb.max());
			const auto textureIterator = preparedTextures.begin() + (&primitive - &primitives.front()) * 6;

			if (primitive.rotation == glm::vec3(0.f)) {
				block->modelTextures.insert(block->modelTextures.begin() + block->modelBoxes.size() * 6, textureIterator, textureIterator + 6);
				block->modelBoxes.emplace_back(aabb);
			}
			else {
				auto tetragons = aabb2tetragons(aabb);

				glm::mat4 mat = glm::translate(glm::mat4(1.f), primitive.origin);
				if (primitive.axis.x) mat = glm::rotate(mat, glm::radians(primitive.rotation.x) * -1.f, glm::vec3(1.f, 0.f, 0.f));
				if (primitive.axis.y) mat = glm::rotate(mat, glm::radians(primitive.rotation.y), glm::vec3(0.f, 1.f, 0.f));
				if (primitive.axis.z) mat = glm::rotate(mat, glm::radians(primitive.rotation.z) * -1.f, glm::vec3(0.f, 0.f, 1.f));
				mat = glm::translate(mat, -primitive.origin);

				for (glm::vec3& vec : tetragons){
					vec = mat * glm::vec4(vec, 1.f);
				}

				block->modelExtraPoints.insert(block->modelExtraPoints.end(), tetragons.begin(), tetragons.end());
				block->modelTextures.insert(block->modelTextures.end(), textureIterator, textureIterator + 6);
			}
		}
		for (std::string& textureName : block->modelTextures) {
			textureName = blockName + '_' + textureName;
		}

		{ // delete old textures
			std::string searchString(blockName + '_');
			if (!fs::is_directory(texturesPath)) fs::create_directories(texturesPath);
			for (const auto& elem : fs::directory_iterator(texturesPath)) {
				if (elem.is_regular_file() && elem.path().extension() == ".png" && elem.path().stem().string().find(searchString) == 0) {
					fs::remove(elem);
				}
			}
		}

		for (const auto& pair : croppedTextures) { // write new textures
			const UVRegion& textureUV = blocksAtlas->get(pair.first);
			const glm::ivec2 globalPosition = glm::ivec2(textureUV.u1 * blocksTexture->getWidth(), textureUV.v1 * blocksTexture->getHeight());
			const glm::ivec2 globalSize = glm::ivec2(textureUV.u2 * blocksTexture->getWidth(), textureUV.v2 * blocksTexture->getHeight()) - globalPosition;

			for (const auto& croppedTexture : pair.second) {
				glm::ivec2 localPosition = glm::ivec2(croppedTexture.uv.u1 * globalSize.x, croppedTexture.uv.v1 * globalSize.y);
				glm::ivec2 localSize = glm::ivec2(croppedTexture.uv.u2 * globalSize.x, croppedTexture.uv.v2 * globalSize.y) - localPosition;
				localPosition += globalPosition;

				glm::bvec2 flip(false, false);

				for (size_t i = 0; i < 2; i++) {
					if (localSize[i] < 0) {
						localSize[i] *= -1;
						localPosition[i] -= localSize[i];
						flip[i] = true;
					}
				}
				localSize = glm::max(localSize, glm::ivec2(1));

				std::unique_ptr<ubyte[]> data(new ubyte[localSize.x * localSize.y * 4]);

				size_t location = localPosition.y * blocksTexture->getWidth() * 4 + localPosition.x * 4;

				for (size_t i = 0; i < localSize.y; i++) {
					memcpy(&data[i * (localSize.x * 4)], &imageData->getData()[location], localSize.x * 4);
					location += blocksTexture->getWidth() * 4;
				}

				std::unique_ptr<ImageData> image(new ImageData(ImageFormat::rgba8888, localSize.x, localSize.y, std::move(data)));

				if (flip.x) image->flipX();
				if (flip.y) image->flipY();
				const size_t rotation = abs(croppedTexture.rotation) % 360;
				if (rotation) {
					rotateImage(image, rotation);
				}

				imageio::write(fs::path(texturesPath / (blockName + '_' + croppedTexture.name + ".png")).string(), image.get());
			}
		}
	}
	catch (const std::exception& e) {
		logger.error() << e.what();
		return block;
	}
	return block;
}

static void rotateImage(std::unique_ptr<ImageData>& image, size_t angleDegrees) {
	if (image->getFormat() != ImageFormat::rgba8888) return;

	uint width = image->getWidth();
	uint height = image->getHeight();

	const int length = width * height * 4;

	uint32_t* original = reinterpret_cast<uint32_t*>(image->getData());
	uint32_t* copy = new uint32_t[length];
	memcpy(copy, original, length);

	if (angleDegrees == 180) {
		std::reverse_copy(copy, copy + width * height, original);
	}
	else if (angleDegrees == 90 || angleDegrees == 270) {

		if (width != height) {
			image.reset(new ImageData(ImageFormat::rgba8888, height, width, new ubyte[length]));
			original = reinterpret_cast<uint32_t*>(image->getData());
		}
		bool clockWise = angleDegrees == 90;

		for (size_t y = 0; y < height; y++) {
			for (size_t x = 0; x < width; x++) {
				original[(x + 1) * height - y - 1] = copy[clockWise ? y * width + x : width * height - 1 - (y * width + x)];
			}
		}
	}
	delete[] copy;
}