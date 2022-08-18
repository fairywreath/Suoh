#include "Logger.h"

#include <sstream>

namespace Suoh
{

std::ostream* Logger::mOutputStream = nullptr;
std::mutex Logger::mOutputMutex;

void Logger::setOutput(std::ostream* output)
{
    auto&& lock __attribute__((unused)) = std::lock_guard<std::mutex>(mOutputMutex);

    mOutputStream = output;
}

Logger& Logger::getInstance()
{
    static auto&& logger = Logger();
    return (logger);
}

} // namespace Suoh
