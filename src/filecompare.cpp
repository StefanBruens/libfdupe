
#include <algorithm>
#include <iostream>
#include <files.h>
#include <prefixtree.h>
#include <memory>

namespace LibFDupe {

template<class T>
void
removeDuplicates(T begin, T end)
{
    typedef typename T::value_type value_type;

    // sort by size and (device, inode)
    std::sort(begin, end, [](const value_type& a, const value_type& b)
    {
        if (a->size() < b->size()) return true;
        if (a->size() > b->size()) return false;
        if (a->inode() < b->inode()) return true;
        if (a->inode() > b->inode()) return false;
        return (a->device() < b->device());
    });

    auto startrange = begin;
    auto endrange = startrange;
    endrange++;

    while(startrange != end) {
        while ((endrange != end) && ((*endrange)->size() == (*startrange)->size()))
            endrange++;
        if (std::distance(startrange, endrange) > 1) {

            std::cout << "Files: " << (*startrange)->size() << " bytes * "
                << std::distance(startrange, endrange) << std::endl;

            //std::for_each(startrange, endrange,
                //[](const value_type& f) { std::cout << f->size() << " " << *f << std::endl; });

            if (std::distance(startrange, endrange) == 1) {

            } else {
                // create PrefixTree
                // collect Files from tree
                PrefixTree tree;
                std::for_each(startrange, endrange, [&tree](value_type& f)
                {
                    tree.insertFile(HashFile(f.get()), 0);
                });
            }
            std::cout << "---" << std::endl;

        }
        startrange = endrange;
        endrange++;
    }
}

template
void
removeDuplicates< std::vector<std::unique_ptr<File>>::iterator >(
    std::vector<std::unique_ptr<File>>::iterator begin,
    std::vector<std::unique_ptr<File>>::iterator end);

} // namespace
