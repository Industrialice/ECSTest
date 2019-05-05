#include "PreHeader.hpp"
#include "RendererDX11.hpp"
#include "Components.hpp"

using namespace ECSEngine;

class RendereDX11SystemImpl : public RendererDX11System
{
public:
    virtual void ProcessMessages(Environment &env, const MessageStreamEntityAdded &stream) override
    {
        for (auto &entry : stream)
        {
            auto camera = entry.FindComponent<Camera>();
            if (camera)
            {
                env.logger.Message(LogLevels::Info, "Received camera component, far plane %f near plane %f\n", camera->farPlane, camera->nearPlane);
                auto window = std::get<Window>(camera->rts[0].target);
                env.logger.Message(LogLevels::Info, "Window width %i height %i\n", window.width, window.height);
            }
        }
    }
    
    virtual void ProcessMessages(Environment &env, const MessageStreamComponentAdded &stream) override
    {
    }
    
    virtual void ProcessMessages(Environment &env, const MessageStreamComponentChanged &stream) override
    {
    }
    
    virtual void ProcessMessages(Environment &env, const MessageStreamComponentRemoved &stream) override
    {
    }
    
    virtual void ProcessMessages(Environment &env, const MessageStreamEntityRemoved &stream) override
    {
    }
    
    virtual void Update(Environment &env) override
    {
    }
};

unique_ptr<Renderer> RendererDX11System::New()
{
    return make_unique<RendereDX11SystemImpl>();
}
