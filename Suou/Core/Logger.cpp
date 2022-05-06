#include "Logger.h"

#include <sstream>

namespace Suou
{

std::ostream* Logger::mOutputStream = nullptr;
std::mutex Logger::mOutputMutex;

void Logger::setOutput(std::ostream* output) 
{
    auto &&lock __attribute__((unused)) = std::lock_guard<std::mutex>(mOutputMutex);

    mOutputStream = output;
}

Logger& Logger::getInstance() 
{
    static auto &&logger = Logger();
    return (logger);
}

}
