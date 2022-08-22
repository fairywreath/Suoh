#include <iostream>
#include <stdexcept>

#include <Core/Logger.h>

#include "Application.h"

using namespace Suoh;

int main()
{
    try
    {
        LOG_SET_OUTPUT(&std::cout);
        LOG_DEBUG("Starting application...");

        Application app;
        app.run();
    }
    catch (std::exception& e)
    {
        LOG_FATAL("EXCEPTION: ", e.what());
    }

    return 0;
}
