#include <iostream>

#include <Logger.h>

#include "Application.h"

int main()
{
    LOG_SET_OUTPUT(&std::cout);
    LOG_DEBUG("Starting application...");

    Application app;
    app.Run();

    return 0;
}

