#include "SystemCommand.h"

namespace SystemCommand
{

static std::array<char, 8192> g_Buffer;

Result Execute(const std::string& command)
{
    int exitcode = 0;
    std::string result;

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#define WEXITSTATUS
#endif
    FILE* pipe = popen(command.c_str(), "r");
    if (pipe == nullptr)
    {
        return Result{"", -1};
    }

    std::size_t bytesread;
    while ((bytesread = std::fread(g_Buffer.data(), sizeof(g_Buffer.at(0)), sizeof(g_Buffer), pipe))
           != 0)
    {
        result += std::string(g_Buffer.data(), bytesread);
    }

    pclose(pipe);
    exitcode = WEXITSTATUS(pclose(pipe));
    return Result{result, exitcode};
}

} // namespace SystemCommand