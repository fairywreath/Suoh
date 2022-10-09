#pragma once

#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <Core/Types.h>
#include <Core/Utils.h>

namespace Suoh
{

void saveStringArray(std::ofstream& file, const std::vector<std::string>& arr);
void loadStringArray(std::ifstream& file, std::vector<std::string>& arr);

void saveMap(std::ofstream& file, const std::unordered_map<u32, u32>& map);
void loadMap(std::ifstream& file, std::unordered_map<u32, u32>& map);

inline int addUnique(std::vector<std::string>& files, const std::string& file)
{
    if (file.empty())
        return -1;

    auto i = std::find(std::begin(files), std::end(files), file);

    if (i == files.end())
    {
        files.push_back(file);
        return (int)files.size() - 1;
    }

    return (int)std::distance(files.begin(), i);
}

} // namespace Suoh
