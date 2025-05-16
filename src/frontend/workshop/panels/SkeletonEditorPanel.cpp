#include "frontend/workshop/WorkshopScreen.hpp"

#include "objects/rigging.hpp"
#include "graphics/ui/elements/Panel.hpp"
#include "graphics/ui/elements/Button.hpp"
#include "graphics/ui/elements/Image.hpp"
#include "graphics/ui/elements/Label.hpp"
#include "util/stringutil.hpp"
#include "frontend/workshop/gui_elements/BasicElements.hpp"
#include "frontend/workshop/WorkshopPreview.hpp"
#include "frontend/workshop/WorkshopSerializer.hpp"

using namespace workshop;

static InputMode inputMode = InputMode::SLIDER;

static void updateIndices(std::vector<rigging::Bone*>& nodes, rigging::Bone* node) {
	nodes.emplace_back(node);
	for (auto& subnode : node->getSubnodes()) {
		updateIndices(nodes, subnode.get());
	}
}

void WorkShopScreen::createSkeletonEditorPanel(rigging::SkeletonConfig& skeleton, std::vector<size_t> skeletonPath) {
	createPanel([this, &skeleton, skeletonPath]() {
		const std::string actualName(getDefName(skeleton.getName()));

		auto getBone = [&skeleton](const std::vector<size_t>& path) {
			rigging::Bone* result = skeleton.getRoot();
			for (size_t i : path){
				result = const_cast<rigging::Bone*>(result->getSubnodes()[i].get());
			}
			return result;
		};

		auto goTo = [this, &skeleton](std::vector<size_t> skeletonPath) {
			createSkeletonEditorPanel(skeleton, skeletonPath);
		};

		rigging::Bone& currentBone = *getBone(skeletonPath);
		const bool root = skeletonPath.empty();

		gui::Panel& panel = *new gui::Panel(glm::vec2(settings.itemEditorWidth));

		panel << new gui::Label(actualName);

		panel << new gui::Button(L"Add bone", glm::vec4(10.f), [sp = skeletonPath, goTo, &currentBone, &skeleton](gui::GUI*) mutable {
			currentBone.bones.emplace_back(new rigging::Bone(0, "", "", std::vector<std::unique_ptr<rigging::Bone>>(), glm::vec3(0.f)));
			sp.emplace_back(currentBone.bones.size() - 1);
			skeleton.nodes.clear();
			updateIndices(skeleton.nodes, skeleton.root.get());
			goTo(sp);
		});
		if (!root) {
			panel << new gui::Button(L"Remove current", glm::vec4(10.f), [goTo, getBone, sp = skeletonPath, &currentBone, &skeleton](gui::GUI*) mutable {
				const size_t removeIndex = sp.back();
				sp.pop_back();
				rigging::Bone* bone = getBone(sp);
				bone->bones.erase(bone->bones.begin() + removeIndex);
				skeleton.nodes.clear();
				updateIndices(skeleton.nodes, skeleton.root.get());
				goTo(sp);
			});
			panel << new gui::Button(L"Back", glm::vec4(10.f), [sp = skeletonPath, goTo](gui::GUI*) mutable {
				sp.pop_back();
				goTo(sp);
			});
		}

		panel << new gui::Label("Sub-Bone List");
		gui::Panel& boneList = *new gui::Panel(glm::vec2(panel.getSize().x, panel.getSize().y / 3.f));
		boneList.setColor(glm::vec4(0.f));
		boneList.listenInterval(0.1f, [&panel, &boneList]() {
			const int maxLength = static_cast<int>(panel.getSize().y / 3.f);
			if (boneList.getMaxLength() != maxLength) {
				boneList.setMaxLength(maxLength);
				boneList.cropToContent();
			}
		});
		if (currentBone.getSubnodes().empty())
			boneList << new gui::Button(L"[empty]", glm::vec4(10.f), gui::onaction());
		for (size_t i = 0; i < currentBone.getSubnodes().size(); i++) {
			const std::unique_ptr<rigging::Bone>& bone = currentBone.getSubnodes()[i];
			const std::string name = bone->getName().empty() ? "[unnamed]" : bone->getName();
			boneList << new gui::Button(util::str2wstr_utf8(name), glm::vec4(10.f), [sp = skeletonPath, i, goTo](gui::GUI*) mutable {
				sp.emplace_back(i);
				goTo(sp);
			});
		}
		panel << boneList;

		panel << new gui::Label("Name");
		createTextBox(panel, currentBone.name);

		panel << new gui::Label("Offset (-10 - 10)");

		gui::Panel& inputPanel = *new gui::Panel(glm::vec2(panel.getSize().x));
		inputPanel.setColor(glm::vec4(0.f));
		panel << inputPanel;

		auto createInput = [&inputPanel, &currentBone](InputMode inputMode) {
			removeRemovable(inputPanel);
			inputPanel << markRemovable(createVectorPanel(currentBone.offset, glm::vec3(-10.f), glm::vec3(10.f), inputPanel.getSize().x, inputMode, runnable()));
		};

		std::wstring inputModes[] = { L"Slider", L"InputBox" };
		gui::Button* button = new gui::Button(L"Input mode: " + inputModes[static_cast<int>(inputMode)], glm::vec4(10.f), gui::onaction());
		button->listenAction([button, inputModes, createInput](gui::GUI*) {
			incrementEnumClass(inputMode, 1);
			if (inputMode > InputMode::TEXTBOX) inputMode = InputMode::SLIDER;
			button->setText(L"Input mode: " + inputModes[static_cast<int>(inputMode)]);
			createInput(inputMode);
		});
		panel << button;
		createInput(inputMode);

		auto modelName = [&currentBone]() {
			return currentBone.model.name.empty() ? NOT_SET : currentBone.model.name;
		};
		panel << new gui::Label("Model");
		button = new gui::Button(util::str2wstr_utf8(modelName()), glm::vec4(10.f), gui::onaction());
		button->listenAction([this, button, modelName, &currentBone, &panel, &skeleton](gui::GUI*) {
			createModelsList(true, 4, panel.calcPos().x + panel.getSize().x, [this, button, modelName, &currentBone, &skeleton](const std::string& string) {
				currentBone.model.name = string;
				currentBone.model.updateFlag = true;
				currentBone.model.refresh(*assets);
				preview->setSkeleton(&skeleton);
				button->setText(util::str2wstr_utf8(modelName()));
				removePanel(4);
			});
		});
		panel << button;

		panel << new gui::Button(L"Save", glm::vec4(10.f), [this, &skeleton, actualName](gui::GUI*) {
			skeletons[skeleton.getName()] = stringify(toJson(*skeleton.getRoot(), actualName), false);
			saveSkeleton(*skeleton.getRoot(), currentPack.folder, actualName);
		});
		panel << new gui::Button(L"Rename", glm::vec4(10.f), [this, actualName](gui::GUI*) {
			createDefActionPanel(ContentAction::RENAME, ContentType::SKELETON, actualName);
		});
		panel << new gui::Button(L"Delete", glm::vec4(10.f), [this, actualName](gui::GUI*) {
			createDefActionPanel(ContentAction::DELETE, ContentType::SKELETON, actualName);
		});

		currentBone.model.refresh(*assets);
		preview->setSkeleton(&skeleton);
		createSkeletonPreview(3);

		return std::ref(panel);
	}, 2);
}
