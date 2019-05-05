#pragma once

#include "Renderer.hpp"

namespace ECSEngine
{
    class RendererDX11System : public Renderer
    {
    protected:
        RendererDX11System() = default;

    public:
        static unique_ptr<Renderer> New();
    };
}