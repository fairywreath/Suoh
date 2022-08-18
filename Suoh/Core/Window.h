#pragma once

#include <string>

#include "CoreTypes.h"

namespace Suoh
{

struct WindowProperties
{
    u32 width;
    u32 height;
    std::string title;
};

class Window
{
public:
    Window(){};
    virtual ~Window(){};

    virtual bool init(const WindowProperties& props) = 0;
    virtual void destroy() = 0;
    virtual void close() = 0;
    virtual void update() = 0;

    virtual bool shouldClose() const = 0;
    virtual void resize(u32 width, u32 height) = 0;

    virtual u32 getWidth() const = 0;
    virtual u32 getHeight() const = 0;

    virtual void* getNativeWindow() const = 0;

protected:
};

} // namespace Suoh