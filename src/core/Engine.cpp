#include "core/Engine.h"

#include <iostream>

namespace n3d
{
int Engine::run()
{
    std::cout << "OpenNitemare3D clean-room runtime bootstrap\n";

    renderer_.initialize();
    renderer_.renderFrame();
    renderer_.shutdown();

    return 0;
}
}
