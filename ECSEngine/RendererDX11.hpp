#pragma once

#include "Renderer.hpp"

namespace ECSEngine
{
    class RendererDX11System : public Renderer, public TypeIdentifiable<RendererDX11System>
    {
    protected:
        RendererDX11System() = default;

    public:
		[[nodiscard]] virtual TypeId GetTypeId() const override
		{
			return TypeIdentifiable<RendererDX11System>::GetTypeId();
		}

		[[nodiscard]] virtual string_view GetTypeName() const override
		{
			return TypeIdentifiable<RendererDX11System>::GetTypeName();
		}

        static unique_ptr<Renderer> New();
    };
}