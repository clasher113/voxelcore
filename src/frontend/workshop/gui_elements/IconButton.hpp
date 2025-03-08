#pragma once

#include "graphics/ui/elements/Container.hpp"

class Atlas;
class Texture;
struct UVRegion;

namespace gui {
	class Image;
	class Label;

	class IconButton : public Container {
	public:
		IconButton(glm::vec2 size, const std::string& text, Texture* const texture, const UVRegion& uv, const std::string& additionalText = "");
		IconButton(glm::vec2 size, const std::string& text, const Atlas* atlas, const std::string& textureName, const std::string& additionalText = "");

		void setIcon(const Atlas* const atlas, const std::string& textureName);
		void setIcon(Texture* const texture);
		void setText(const std::string& text);

	private:

		std::shared_ptr<Label> label;
		std::shared_ptr<Image> image;
	};
}