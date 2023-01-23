#pragma once

#include <array>
#include <ostream>
#include <string>
#ifdef _WIN32
#include <stdio.h>
#endif

namespace SystemCommand
{

struct Result
{
    std::string output;
    int exitstatus;

    friend std::ostream& operator<<(std::ostream& os, const Result& result)
    {
        os << "command exitstatus: " << result.exitstatus << " output: " << result.output;
        return os;
    }
    bool operator==(const Result& rhs) const
    {
        return output == rhs.output && exitstatus == rhs.exitstatus;
    }
    bool operator!=(const Result& rhs) const
    {
        return !(rhs == *this);
    }
};

Result Execute(const std::string& command);

}; // namespace SystemCommand
