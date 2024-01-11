#ifndef SRC_CONSTANTS_H_
#define SRC_CONSTANTS_H_

#include <limits>
#include "typedefs.h"

const int ENGINE_VERSION_MAJOR = 0;
const int ENGINE_VERSION_MINOR = 16;

const int CHUNK_W = 16;
const int CHUNK_H = 256;
const int CHUNK_D = 16;

/* Chunk volume (count of voxels per Chunk) */
const int CHUNK_VOL = (CHUNK_W * CHUNK_H * CHUNK_D);

/* BLOCK_VOID is block id used to mark non-existing voxel (voxel of missing chunk) */
const blockid_t BLOCK_VOID = std::numeric_limits<blockid_t>::max();
const blockid_t MAX_BLOCKS = BLOCK_VOID;

inline uint vox_index(int x, int y, int z, int w=CHUNK_W, int d=CHUNK_D) {
	return (y * d + z) * w + x;
}

//cannot replace defines with const while used for substitution
#ifdef USE_DIRECTX
#define SHADERS_FOLDER "dx-shaders" 
#elif USE_OPENGL
#define SHADERS_FOLDER "shaders" 
#endif // USE_DIRECTX
#define TEXTURES_FOLDER "textures"
#define FONTS_FOLDER "fonts"

#endif // SRC_CONSTANTS_H_
