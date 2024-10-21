#include "IconButton.hpp"

#include "../../../graphics/core/Atlas.hpp"
#include "../../../graphics/ui/elements/Image.hpp"
#include "../../../graphics/ui/elements/Label.hpp"
#include "../../../util/stringutil.hpp"
#include "../WorkshopUtils.hpp"

gui::IconButton::IconButton(glm::vec2 size, const std::string& text, Texture* const texture, const UVRegion& uv, const std::string& additionalText) : Container(size),
label(new gui::Label(text)),
image(new gui::Image(texture))
{
	setColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.95f));
	setHoverColor(glm::vec4(0.05f, 0.1f, 0.15f, 0.75f));
	workshop::formatTextureImage(*image, texture, size.y, uv);

	if (additionalText.empty()) {
		label->setPos(glm::vec2(size.y + 10.f, size.y / 2.f - label->getSize().y / 2.f));
	}
	else {
		label->setPos(glm::vec2(size.y + 10.f, size.y / 2.f));
		auto additionalLabel = std::make_shared<gui::Label>(additionalText);
		additionalLabel->setPos(glm::vec2(size.y + 10.f, size.y / 2.f - label->getSize().y));
		add(additionalLabel);
	}
	add(label);
	add(image);
}

gui::IconButton::IconButton(glm::vec2 size, const std::string& text, const Atlas* atlas, const std::string& textureName, const std::string& additionalText) :
	IconButton(size, text, atlas->getTexture(), UVRegion(), additionalText)
{
	workshop::formatTextureImage(*image, atlas->getTexture(), size.y, atlas->get(textureName));
}

void gui::IconButton::setIcon(const Atlas* atlas, const std::string& textureName) {
	image->setTexture(atlas->getTexture());
	workshop::formatTextureImage(*image, atlas->getTexture(), size.y, atlas->get(textureName));
}

void gui::IconButton::setIcon(Texture* const texture) {
	image->setTexture(texture);
	image->setSize(glm::vec2(size.y));
}

void gui::IconButton::setText(const std::string& text) {
	label->setText(util::str2wstr_utf8(text));
}
