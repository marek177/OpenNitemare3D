#include "core/Engine.h"

#include <exception>
#include <iostream>

int main()
{
    try
    {
        n3d::Engine engine;
        return engine.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "OpenNitemare3D fatal error: " << e.what() << '\n';
        return 1;
    }
}
