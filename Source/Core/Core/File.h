#pragma once

#include <string>
#include <vector>

#include "Types.h"

namespace File
{

template <typename T> size_t ReadBinaryFile(const std::string& fileName, std::vector<T>& buffer);

}