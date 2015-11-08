#ifndef __FDUPE_FILELIST_H__
#define __FDUPE_FILELIST_H__

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <sys/stat.h>

namespace LibFDupe {

class TraversalFlags {
public:
    enum TraversalFlag {
        None = 0,
        FollowSymlinks = 1,
        CrossDevice = 2,
        CrossDeviceTarget = 4,
        IgnoreEmptyFiles = 8
    };

    typedef std::underlying_type<TraversalFlag>::type valueType;

    TraversalFlags() : m_value(0) {};
    TraversalFlags(const valueType& value) : m_value(value) {};
    TraversalFlags& operator|(const TraversalFlags& other) {
        m_value |= other.m_value;
        return *this;
    }
    bool operator&(const TraversalFlags& other) {
        return m_value & other.m_value;
    }

private:
    valueType m_value;
};

class DirEnt;
class Directory;
class File;
class Symlink;

class FileFinder
{
public:
    FileFinder() = default;
    FileFinder(const std::string& path, TraversalFlags flags);
    int findFiles(const std::string& path, TraversalFlags flags);
    int findFiles(const char* path, TraversalFlags flags);

    std::vector<std::unique_ptr<File>> files();

private:
    TraversalFlags m_flags;

    int traverseDirectory(int dirfd, const Directory* parentDir);
    const DirEnt* examineSymlink(int dirfd, const Directory* parentDir, const std::string& target);
    const DirEnt* examinePathComponent(int dirfd, const Directory* parentDir, std::vector<std::string> components);

    std::map< std::pair<ino_t, dev_t>, std::unique_ptr<const Directory>> m_seenDirectories;
    std::map< std::pair<ino_t, dev_t>, std::unique_ptr<const Symlink>> m_seenLinks;
    std::multimap<off_t, std::unique_ptr<File>> m_seenFiles;
};

} // namespace LibFDupe

#endif // __FDUPE_FILELIST_H__

