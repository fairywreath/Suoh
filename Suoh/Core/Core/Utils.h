#pragma once

namespace Suoh
{

template <typename T> inline void mergeVectors(std::vector<T>& v1, const std::vector<T>& v2)
{
    v1.insert(v1.end(), v2.begin(), v2.end());
}

// From https://stackoverflow.com/a/64152990/1182653
// Delete a list of items from std::vector with indices in 'selection'
template <class T, class Index = int> inline void eraseSelected(std::vector<T>& v, const std::vector<Index>& selection)
// e.g., eraseSelected({1, 2, 3, 4, 5}, {1, 3})  ->   {1, 3, 5}
//                         ^     ^    2 and 4 get deleted
{
    // cut off the elements moved to the end of the vector by std::stable_partition
    v.resize(std::distance(
        v.begin(),
        // the stable_partition moves any element whose index is in 'selection' to the end
        std::stable_partition(v.begin(), v.end(), [&selection, &v](const T& item) {
            return !std::binary_search(
                selection.begin(), selection.end(),
                /* std::distance(std::find(v.begin(), v.end(), item), v.begin()) - if you don't like the pointer arithmetic below */
                static_cast<Index>(static_cast<const T*>(&item) - &v[0]));
        })));
}

} // namespace Suoh