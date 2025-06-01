#include "IconButton.hpp"

#include "assets/Assets.hpp"
#include "graphics/core/Atlas.hpp"
#include "graphics/ui/elements/Image.hpp"
#include "graphics/ui/elements/Label.hpp"
#include "util/stringutil.hpp"
#include "frontend/workshop/WorkshopUtils.hpp"

gui::IconButton::IconButton(const Assets* const assets, float height, const std::string& text, const std::string& textureName, const std::string& additionalText) : Container(size),
label(new gui::Label(text)),
image(new gui::Image(textureName))
{
	size.y = height;
	setScrollable(false);
	setColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.95f));
	setHoverColor(glm::vec4(0.05f, 0.1f, 0.15f, 0.75f));
	workshop::formatTextureImage(*image, textureName, height, assets);

	if (additionalText.empty()) {
		label->setPos(glm::vec2(height + 10.f, height / 2.f - label->getSize().y / 2.f));
	}
	else {
		label->setPos(glm::vec2(height + 10.f, height / 2.f));
		auto additionalLabel = std::make_shared<gui::Label>(additionalText);
		additionalLabel->setPos(glm::vec2(height + 10.f, height / 2.f - label->getSize().y));
		add(additionalLabel);
	}
	add(label);
	add(image);
}

void gui::IconButton::setIcon(const Assets* const assets, const std::string& atlasAndTexture) {
	workshop::formatTextureImage(*image, atlasAndTexture, size.y, assets);
}

void gui::IconButton::setText(const std::string& text) {
	label->setText(util::str2wstr_utf8(text));
}
