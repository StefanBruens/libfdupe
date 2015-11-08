
#include <files.h>
#include <filefinder.h>
#include <filecompare.h>

int main(int argc, char* argv[])
{
    using namespace LibFDupe;

    FileFinder list;
    if (argc < 2)
        return -1;

    list.findFiles(argv[1], TraversalFlags::FollowSymlinks | TraversalFlags::IgnoreEmptyFiles);
    auto files = list.files();
    removeDuplicates(files.begin(), files.end());

    return 0;
}

