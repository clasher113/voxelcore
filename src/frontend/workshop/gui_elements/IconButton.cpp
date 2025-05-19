#include "IconButton.hpp"

#include "assets/Assets.hpp"
#include "graphics/core/Atlas.hpp"
#include "graphics/ui/elements/Image.hpp"
#include "graphics/ui/elements/Label.hpp"
#include "util/stringutil.hpp"
#include "frontend/workshop/WorkshopUtils.hpp"

gui::IconButton::IconButton(float height, const std::string& text, Texture* const texture, const UVRegion& uv, const std::string& additionalText) : Container(size),
label(new gui::Label(text)),
image(new gui::Image(texture))
{
	size.y = height;
	setScrollable(false);
	setColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.95f));
	setHoverColor(glm::vec4(0.05f, 0.1f, 0.15f, 0.75f));
	workshop::formatTextureImage(*image, texture, height, uv);

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

gui::IconButton::IconButton(float height, const std::string& text, const Atlas* const atlas, const std::string& textureName, const std::string& additionalText) :
	IconButton(height, text, atlas->getTexture(), UVRegion(), additionalText)
{
	setIcon(atlas, textureName);
}

gui::IconButton::IconButton(float height, const std::string& text, const Assets* const assets, const std::string& atlasAndTexture, const std::string& additionalText) :
	IconButton(height, text, assets->get<Atlas>(workshop::getAtlasName(atlasAndTexture))->getTexture(), UVRegion(), additionalText)
{
	setIcon(assets, atlasAndTexture);
}

void gui::IconButton::setIcon(const Assets* const assets, const std::string& atlasAndTexture) {
	setIcon(assets->get<Atlas>(workshop::getAtlasName(atlasAndTexture)), atlasAndTexture);
}

void gui::IconButton::setIcon(const Atlas* const atlas, const std::string& textureName) {
	image->setTexture(atlas->getTexture());
	workshop::formatTextureImage(*image, atlas->getTexture(), size.y, atlas->get(atlas->has(textureName) ? textureName : workshop::getTexName(textureName)));
}

void gui::IconButton::setIcon(Texture* const texture) {
	image->setTexture(texture);
	image->setSize(glm::vec2(size.y));
}

void gui::IconButton::setText(const std::string& text) {
	label->setText(util::str2wstr_utf8(text));
}
