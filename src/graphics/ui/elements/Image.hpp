#pragma once

#include "UINode.hpp"
#include "../../../maths/UVRegion.hpp"

class Texture;

namespace gui {
    class Image : public UINode {
        Texture* tex = nullptr;
        UVRegion uv;
    protected:
        std::string texture;
        bool autoresize = false;
    public:
        Image(std::string texture, glm::vec2 size=glm::vec2(32,32));
        Image(Texture* texture, glm::vec2 size=glm::vec2(32,32));

        void setTexture(Texture* const texture) { tex = texture; };
        void setUVRegion(const UVRegion& uv) { this->uv = uv; };

        virtual void draw(const DrawContext& pctx, const Assets& assets) override;

        virtual void setAutoResize(bool flag);
        virtual bool isAutoResize() const;
        virtual const std::string& getTexture() const;
        virtual void setTexture(const std::string& name);
    };
}
