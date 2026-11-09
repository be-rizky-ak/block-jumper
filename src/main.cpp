#include <iostream>

#include "game/BlockJumperApp.h"

int main()
{
    BlockJumper::BlockJumperApp app;

    if (!app.IsRunning())
    {
        std::cerr << "[BlockJumper] Error: Failed to initialize application graphics context." << std::endl;
        std::cerr << "[BlockJumper] Ensure an OpenGL 3.3 compatible context and display are available." << std::endl;
        return 1;
    }

    app.Run();

    return 0;
}
