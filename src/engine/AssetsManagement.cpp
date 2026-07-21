#include "AssetsManagement.hpp"

#include "assets/AssetsLoader.hpp"
#include "content/Content.hpp"
#include "debug/Logger.hpp"
#include "engine/Engine.hpp"
#include "EnginePaths.hpp"
#include "graphics/render/ModelsGenerator.hpp"
#include "graphics/ui/GUI.hpp"

#ifdef USE_DIRECTX
#include "directx/ShaderInclude.hpp"
#include "directx/graphics/Shader.hpp"
#elif USE_OPENGL
#include "coders/GLSLExtension.hpp"
#include "graphics/core/Shader.hpp"
#endif // USE_DIRECTX


static debug::Logger logger("assets-management");

AssetsManagement::AssetsManagement(Engine& engine)
    : engine(engine), settings(engine.getSettings()) {
}

AssetsManagement::~AssetsManagement() {
    finishBackgroundLoader();
    assets.reset();
    assetsVault.clearKeepedObjects();
}

const Assets* AssetsManagement::getStorage() const {
    return assets.get();
}

Assets* AssetsManagement::getStorage() {
    return assets.get();
}

AssetsLoader& AssetsManagement::acquireBackgroundLoader() {
    if (backgroundLoader) {
        return *backgroundLoader;
    }
    if (assets == nullptr) {
        throw std::runtime_error("no assets storage available");
    }
    backgroundLoader = std::make_unique<AssetsLoader>(
        engine, *assets, engine.getResPaths()
    );
    backgroundLoaderTask = backgroundLoader->startTask(
        nullptr, settings.system.maxBgAssetLoaders.get()
    );
    return *backgroundLoader;
}

void AssetsManagement::loadAssets(Content* content) {
    finishBackgroundLoader();

    const auto& paths = engine.getPaths();
    logger.info() << "loading assets";
#ifdef USE_DIRECTX
    ShaderInclude::setPaths(&paths.resPaths);
#elif USE_OPENGL
    Shader::preprocessor->setPaths(&paths.resPaths);
#endif // USE_DIRECTX

    auto new_assets = std::make_unique<Assets>(
        settings.system.preserveAssetsDuringFrame.get() ? &assetsVault : nullptr 
    );
    AssetsLoader loader(engine, *new_assets, paths.resPaths);
    AssetsLoader::addDefaults(loader, content);

    // no need
    // correct log messages order is more useful
    // todo: before setting to true, check if GLSLExtension thread safe 
    bool threading = false; // look at three upper lines
    if (threading) {
        auto task = loader.startTask(
            [=]() {}, 0
        );
        task->waitForEnd();
    } else {
        while (loader.hasNext()) {
            loader.loadNext();
        }
    }
    assets = std::move(new_assets);
    if (content) {
        ModelsGenerator::prepare(*content, *assets);
    }
    assets->setup();
    engine.getGUI().onAssetsLoad(assets.get());
}

void AssetsManagement::update() {
    assetsVault.clearKeepedObjects();
    if (backgroundLoaderTask) {
        backgroundLoaderTask->update();
    }
}

void AssetsManagement::finishBackgroundLoader() {
    if (backgroundLoaderTask == nullptr) {
        return;
    }
    backgroundLoaderTask.reset();
    backgroundLoader.reset();
}
