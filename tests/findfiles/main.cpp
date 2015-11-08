
#include <algorithm>
#include <iostream>
#include <files.h>
#include <filefinder.h>

int main(int argc, char* argv[])
{
    using namespace LibFDupe;

    FileFinder list;
    if (argc < 2)
        return -1;

    list.findFiles(argv[1], TraversalFlags::FollowSymlinks | TraversalFlags::IgnoreEmptyFiles);
    auto files = list.files();

    typedef decltype(*files.begin()) value_type;
    std::for_each(files.begin(), files.end(),
        [](const value_type& f) {
            std::cout << f->size() << " " << *f << std::endl;
        });

    return 0;
}

