#include "frontend/workshop/WorkshopScreen.hpp"

#include "graphics/ui/elements/Panel.hpp"
#include "graphics/ui/elements/Button.hpp"
#include "graphics/ui/elements/CheckBox.hpp"
#include "graphics/ui/elements/TextBox.hpp"
#include "graphics/ui/elements/Image.hpp"
#include "frontend/workshop/gui_elements/BasicElements.hpp"
#include "frontend/workshop/gui_elements/IconButton.hpp"
#include "frontend/workshop/WorkshopPreview.hpp"
#include "presets/ParticlesPreset.hpp"
#include "util/stringutil.hpp"

using namespace workshop;

static InputMode inputMode = InputMode::TEXTBOX;

void WorkShopScreen::createBlockParticlesEditor(gui::Panel& panel, std::unique_ptr<ParticlesPreset>& particles) {
	panel.clear();
	
	panel.setOrientation(gui::Orientation::horizontal);
	panel.setPadding(glm::vec4(0.f));
	panel.setMargin(glm::vec4(0.f));

	gui::Panel& editorPanel = *new gui::Panel(glm::vec2(settings.customModelEditorWidth));
	editorPanel.setMargin(glm::vec4(0.f));
	editorPanel.setColor(glm::vec4(0.f));
	editorPanel.setSizeFunc([&editorPanel, &panel]() {
		return glm::vec2(editorPanel.getSize().x, panel.getSize().y);
	});
	panel << editorPanel;

	gui::Panel& previewPanel = createParticlesPreview();
	previewPanel.setMargin(glm::vec4(0.f));
	previewPanel.setColor(glm::vec4(0.f));
	panel << previewPanel;
	previewPanel.act(1.f);

	if (!particles){
		editorPanel << new gui::Button(L"Create preset", glm::vec4(10.f), [=, &panel, &particles](gui::GUI*) {
			particles = std::make_unique<ParticlesPreset>();
			createBlockParticlesEditor(panel, particles);
		});
		return;
	}

	createFullCheckBox(editorPanel, L"Collision", particles->collision);
	createFullCheckBox(editorPanel, L"Lighting", particles->lighting);
	createFullCheckBox(editorPanel, L"Global up vector", particles->globalUpVector, L"Use global up vector instead of camera-dependent one");

	gui::Button* button = new gui::Button(L"Spawn shape: " + util::str2wstr_utf8(to_string(particles->spawnShape)), glm::vec4(10.f), gui::onaction());
	button->listenAction([=, &particles](gui::GUI*) {
		incrementEnumClass(particles->spawnShape, 1);
		if (particles->spawnShape > ParticleSpawnShape::BOX) particles->spawnShape = ParticleSpawnShape::BALL;
		button->setText(L"Spawn shape: " + util::str2wstr_utf8(to_string(particles->spawnShape)));
	});
	editorPanel << button;

	editorPanel << new gui::Label("Max distance (0.01 - inf)");
	editorPanel << createNumTextBox(particles->maxDistance, L"0.01", 2, 0.01f);
	editorPanel << new gui::Label("Spawn interval (0.0001 - inf)");
	editorPanel << createNumTextBox(particles->spawnInterval, L"0.0001", 4, 0.0001f);
	editorPanel << new gui::Label("Life time (0.01 - inf)");
	editorPanel << createNumTextBox(particles->lifetime, L"0.01", 2, 0.01f);
	editorPanel << new gui::Label("Life time spread (0 - inf)");
	editorPanel << createNumTextBox(particles->lifetimeSpread, L"0.0", 2, 0.f);
	editorPanel << new gui::Label("Size spread (0 - inf)");
	editorPanel << createNumTextBox(particles->sizeSpread, L"0.0", 2, 0.f);
	editorPanel << new gui::Label("Angle spread (-1 - 1)");
	editorPanel << createNumTextBox(particles->angleSpread, L"-1.0", 3, -1.f, 1.f);
	editorPanel << new gui::Label("Min angular velocity");
	editorPanel << createNumTextBox(particles->minAngularVelocity, L"0.0", 3, 0.f);
	editorPanel << new gui::Label("Max angular velocity");
	editorPanel << createNumTextBox(particles->maxAngularVelocity, L"0.0", 3, 0.f);
	editorPanel << new gui::Label("Random sub UV (0.0 - 1.0)");
	editorPanel << createNumTextBox(particles->randomSubUV, L"0.0", 4, 0.f, 1.f);

	constexpr float floatMax = std::numeric_limits<float>::max();
	constexpr float floatLowest = std::numeric_limits<float>::lowest();

	editorPanel << new gui::Label("Initial velocity");
	editorPanel << createVectorPanel(particles->velocity, glm::vec3(floatLowest), glm::vec3(floatMax), editorPanel.getSize().x, inputMode);
	editorPanel << new gui::Label("Acceleration");
	editorPanel << createVectorPanel(particles->acceleration, glm::vec3(floatLowest), glm::vec3(floatMax), editorPanel.getSize().x, inputMode);
	editorPanel << new gui::Label("Explosion");
	editorPanel << createVectorPanel(particles->explosion, glm::vec3(floatLowest), glm::vec3(floatMax), editorPanel.getSize().x, inputMode);
	editorPanel << new gui::Label("Size");
	editorPanel << createVectorPanel(particles->size, glm::vec3(0.f), glm::vec3(floatMax), editorPanel.getSize().x, inputMode);
	editorPanel << new gui::Label("Spawn spread");
	editorPanel << createVectorPanel(particles->spawnSpread, glm::vec3(0.f), glm::vec3(floatMax), editorPanel.getSize().x, inputMode);

	auto getButtonText = [](const std::string& textureName) {
		return textureName == "" ? NOT_SET : textureName;
	};

	auto getTextureTypes = [&particles](const std::string* const current) -> std::vector<ContentType> {
		const std::string* found = nullptr;
		for (const std::string& elem : particles->frames){
			if (!elem.empty() && &elem != current){
				found = &elem;
				break;
			}
		}
		auto extractType = [](std::string string) {
			string = string.substr(0, string.find(':'));
			if (string == "blocks") return ContentType::BLOCK;
			if (string == "items") return ContentType::ITEM;
			if (string == "particles") return ContentType::PARTICLE;
		};
		if (!particles->texture.empty() && &particles->texture != current) return { extractType(particles->texture) };
		else if (found != nullptr) return { extractType(*found) };
		else return { ContentType::BLOCK, ContentType::ITEM, ContentType::PARTICLE };
	};

	editorPanel << new gui::Label("Texture");
	gui::IconButton* iconButton = new gui::IconButton(assets, 35.f, getButtonText(particles->texture), particles->texture);
	iconButton->listenAction([=, &editorPanel, &particles](gui::GUI*) {
		createTextureList(35.f, 5, getTextureTypes(&particles->texture), editorPanel.calcPos().x + editorPanel.getSize().x,
			true, [=, &particles](const std::string& textureName) {
			particles->texture = textureName == "blocks:transparent" ? "" : textureName;
			iconButton->setIcon(assets, particles->texture);
			iconButton->setText(getButtonText(particles->texture));
			removePanel(5);
		});
	});
	editorPanel << iconButton;

	editorPanel << new gui::Label("Animation frames");
	gui::Panel& framesPanel = *new gui::Panel(glm::vec2(settings.customModelEditorWidth));
	framesPanel.setMargin(glm::vec4(0.f));
	framesPanel.setColor(glm::vec4(0.f));
	framesPanel.setMaxLength(400);
	auto createFramesList = [=, &editorPanel, &framesPanel, &particles](const auto self) -> void {
		framesPanel.clear();
		for (size_t i = 0; i < particles->frames.size(); i++){
			std::string& frame = particles->frames[i];
			gui::Container& frameContainer = *new gui::Container(glm::vec2(editorPanel.getSize().x, 35.f));
			frameContainer.setColor(glm::vec4(0.f));
			gui::IconButton* iconButton = new gui::IconButton(assets, 35.f, getButtonText(frame), frame);
			iconButton->setSize(glm::vec2(editorPanel.getSize().x, 35.f));
			iconButton->listenAction([=, &editorPanel, &frame](gui::GUI*) {
				createTextureList(35.f, 5, getTextureTypes(&frame), editorPanel.calcPos().x + editorPanel.getSize().x,
					true, [=, &frame](const std::string& textureName) {
					frame = textureName;
					iconButton->setIcon(assets, frame);
					iconButton->setText(frame);
					removePanel(5);
				});
			});
			frameContainer << iconButton;
			gui::Button* deleteButton = new gui::Button(std::make_shared<gui::Image>("gui/delete_icon"), glm::vec4(0.f));
			deleteButton->setGravity(gui::Gravity::center_right);
			deleteButton->listenAction([=, &particles](gui::GUI*) {
				particles->frames.erase(particles->frames.begin() + i);
				self(self);
			});
			frameContainer << deleteButton;
			framesPanel << frameContainer;

		}
		framesPanel << new gui::Button(L"Add frame", glm::vec4(10.f), [=, &particles](gui::GUI*) {
			particles->frames.emplace_back();
			self(self);
		});
	};
	createFramesList(createFramesList);
	editorPanel << framesPanel;

	editorPanel << new gui::Button(L"Delete preset", glm::vec4(10.f), [=, &panel, &particles](gui::GUI*) {
		particles.reset();
		preview->updateParticles(true);
		createBlockParticlesEditor(panel, particles);
	});

	editorPanel.listenInterval(0.25f, [this]() {
		preview->updateParticles(false);
	});
}