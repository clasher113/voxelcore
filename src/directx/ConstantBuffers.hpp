#ifdef USE_DIRECTX
#ifndef CONSTANT_BUFFERS_HPP
#define CONSTANT_BUFFERS_HPP

#include "ConstantBuffer.hpp"
#include "ConstantBufferTypes.hpp"

extern ConstantBuffer<CB_Main>* cbMain;
extern ConstantBuffer<CB_Lines>* cbLines;
extern ConstantBuffer<CB_UI>* cbUI;
extern ConstantBuffer<CB_UI3D>* cbUI3D;
extern ConstantBuffer<CB_Background>* cbBackground;
extern ConstantBuffer<CB_Skybox>* cbSkybox;

namespace CBuffers {
	extern void initialize();
}
#endif // !CONSTANT_BUFFERS_HPP
#endif // USE_DIRECTX