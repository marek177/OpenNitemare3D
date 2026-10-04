#pragma once

#include "render/Renderer.h"

namespace n3d
{
class Engine
{
public:
    int run();

private:
    Renderer renderer_;
};
}
