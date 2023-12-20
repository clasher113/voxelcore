#ifdef USE_DIRECTX

#include "ConstantBuffers.hpp"

ConstantBuffer<CB_Main>* cbMain;
ConstantBuffer<CB_Lines>* cbLines;
ConstantBuffer<CB_UI>* cbUI;
ConstantBuffer<CB_UI3D>* cbUI3D;
ConstantBuffer<CB_Background>* cbBackground;
ConstantBuffer<CB_Skybox>* cbSkybox;

void CBuffers::initialize() {
	cbMain = new ConstantBuffer<CB_Main>();
	cbLines = new ConstantBuffer<CB_Lines>();
	cbUI = new ConstantBuffer<CB_UI>();
	cbUI3D = new ConstantBuffer<CB_UI3D>();
	cbBackground = new ConstantBuffer<CB_Background>();
	cbSkybox = new ConstantBuffer<CB_Skybox>();
}

#endif // USE_DIRECTX