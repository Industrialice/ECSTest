#pragma once

#include "Renderer.hpp"

namespace ECSEngine
{
    class RendererDX11System : public Renderer, public TypeIdentifiable<RendererDX11System>
    {
    protected:
        RendererDX11System() = default;

    public:
		using TypeIdentifiable<RendererDX11System>::GetTypeId;
		using TypeIdentifiable<RendererDX11System>::GetTypeName;

        static unique_ptr<Renderer> New();
    };
}