#include <iostream>

#include <Logger.h>

#include "Application.h"

using namespace Suou;

int main()
{
    LOG_SET_OUTPUT(&std::cout);
    LOG_DEBUG("Starting client...");

    Application app;
    app.run();

    return 0;
}
