#pragma once

#include <mutex>
#include <ostream>
#include <string>

namespace Suou
{

class Logger
{
public:
    enum class LEVEL
    {
        FATAL = 0,
        ERROR = 1,
        WARN = 2,
        INFO = 3,
        DEBUG = 4,
        TRACE = 5
    };

    ~Logger() = default;

    Logger(const Logger&) = delete;
    void operator=(const Logger&) = delete;
    Logger(Logger&&) noexcept = default;
    Logger& operator=(Logger&&) noexcept = default;

    static Logger& getInstance();
    void setOutput(std::ostream* output);

    template <typename... Args> void log(LEVEL level, Args&&... args)
    {
        // XXX: use std::format when available
        std::ostringstream oss;
        oss << "[";

        switch (level)
        {
        case (LEVEL::FATAL):
            oss << "FATAL";
            break;
        case (LEVEL::ERROR):
            oss << "ERROR";
            break;
        case (LEVEL::WARN):
            oss << "WARN";
            break;
        case (LEVEL::INFO):
            oss << "INFO";
            break;
        case (LEVEL::DEBUG):
            oss << "DEBUG";
            break;
        case (LEVEL::TRACE):
            oss << "TRACE";
            break;
        default:
            return;
        }

        oss << "]: ";
        (oss << ... << std::forward<Args>(args));

        auto&& lock __attribute__((unused)) = std::lock_guard<std::mutex>(mOutputMutex);
        if (mOutputStream != nullptr)
        {
            (*mOutputStream) << oss.str() << std::endl;
        }
    };

private:
    Logger() = default;

    static std::ostream* mOutputStream;
    static std::mutex mOutputMutex;
};

#define LOG_SET_OUTPUT(output) Logger::getInstance().setOutput(output)

#define LOG_FATAL(...) Logger::getInstance().log(Logger::LEVEL::FATAL, __VA_ARGS__)

#define LOG_ERROR(...) Logger::getInstance().log(Logger::LEVEL::ERROR, __VA_ARGS__)

#define LOG_WARN(...) Logger::getInstance().log(Logger::LEVEL::WARN, __VA_ARGS__)

#define LOG_INFO(...) Logger::getInstance().log(Logger::LEVEL::INFO, __VA_ARGS__)

#define LOG_DEBUG(...) Logger::getInstance().log(Logger::LEVEL::DEBUG, __VA_ARGS__)

#define LOG_TRACE(...) Logger::getInstance().log(Logger::LEVEL::TRACE, __VA_ARGS__)

} // namespace Suou
