#include "MenuScreen.hpp"

#include "content/ContentControl.hpp"
#include "graphics/ui/GUI.hpp"
#include "graphics/ui/elements/Menu.hpp"
#include "graphics/core/Batch2D.hpp"
#include "assets/Assets.hpp"
#include "maths/UVRegion.hpp"
#include "window/Window.hpp"
#include "window/Camera.hpp"
#include "engine/Engine.hpp"

#ifdef USE_DIRECTX
#include "directx/graphics/Shader.hpp"
#include "directx/graphics/Texture.hpp"
#elif USE_OPENGL
#include "graphics/core/Shader.hpp"
#include "graphics/core/Texture.hpp"
#endif // USE_DIRECTX

MenuScreen::MenuScreen(Engine& engine)
    : Screen(engine),
      uicamera(
          std::make_unique<Camera>(glm::vec3(), engine.getWindow().getSize().y)
      ) {
    uicamera->perspective = false;
    uicamera->near = -1.0f;
    uicamera->far = 1.0f;
    uicamera->flipped = true;
}

MenuScreen::~MenuScreen() = default;

void MenuScreen::onOpen() {
    engine.getContentControl().resetContent({});
    
    auto menu = engine.getGUI().getMenu();
    menu->reset();
}

void MenuScreen::update(float delta) {
}

void MenuScreen::draw(float delta) {
    display::clear();
    display::setBgColor(glm::vec3(0.2f));
}
