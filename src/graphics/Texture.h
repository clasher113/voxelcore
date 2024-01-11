#ifndef GRAPHICS_TEXTURE_H_
#define GRAPHICS_TEXTURE_H_

#include <string>
#include "../typedefs.h"

class ImageData;

class Texture {
public:
	uint id;

	Texture(uint id, unsigned int width, unsigned int height);
	Texture(ubyte* data, unsigned int width, unsigned int height, uint format);
	~Texture();

	void bind();
	void reload(ubyte* data);

	unsigned int getHeight() { return height; };
	unsigned int getWidth() { return width; };

	static Texture* from(const ImageData* image);

private:
	unsigned int width;
	unsigned int height;
};

#endif /* GRAPHICS_TEXTURE_H_ */
