#pragma once

#include "graphics/ui/elements/Container.hpp"

class Assets;
class Atlas;
class Texture;
struct UVRegion;

namespace gui {
	class Image;
	class Label;

	class IconButton : public Container {
	public:
		IconButton(const Assets* const assets, float height, const std::string& text, const std::string& textureName, const std::string& additionalText = "");

		void setIcon(const Assets* const assets, const std::string& atlasAndTexture);
		void setText(const std::string& text);

	private:

		std::shared_ptr<Label> label;
		std::shared_ptr<Image> image;
	};
}