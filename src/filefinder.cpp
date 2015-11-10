#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <files.h>
#include <filefinder.h>

namespace LibFDupe {

int FileFinder::traverseDirectory(int dirfd, const Directory* dir)
{
    m_seenDirectories.emplace( std::pair<ino_t, dev_t>(dir->inode(), dir->device()),
        std::unique_ptr<const Directory>(dir));

    // create fd suitable for fdopendir
    int readfd = openat(dirfd, ".", O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
    if (readfd <= 0) {
        return readfd;
    }

    DIR* dirstream = fdopendir(readfd);

    if (!dirstream) {
        close(readfd);
        return errno;
    }

    struct dirent* dirinfo;
    while ((dirinfo = readdir(dirstream)) != nullptr) {
        auto type = dirinfo->d_type;
        if ((type != DT_DIR) && (type != DT_REG) &&
            (type != DT_UNKNOWN) && (type != DT_LNK))
            continue;

        std::string name(dirinfo->d_name);
        if ((type != DT_REG) && ((name == ".") || (name == "..")))
            continue;

// FIXME everything below is like examinePath?
        struct stat fileinfo;

        int direntfd = openat(dirfd, dirinfo->d_name, O_PATH | O_NOFOLLOW);
        if (direntfd < 0) {
            std::cerr << "Cant open " << *dir << " " << name << std::endl;
            return direntfd;
        }
        int err = fstatat(direntfd, "", &fileinfo, AT_EMPTY_PATH);
        if (err) {
            closedir(dirstream);
            return err;
        }

        if (S_ISDIR(fileinfo.st_mode)) {

            auto match = m_seenDirectories.count(std::pair<ino_t, dev_t>(fileinfo.st_ino, fileinfo.st_dev));
            if (match) {
                std::cerr << "Known  " << *dir << name << std::endl;
            } else {
                Directory child(name, fileinfo, dir);
                int err = traverseDirectory(direntfd, new Directory(child));
                if (err) {
                    std::cerr << "could not examine " << child << std::endl;
                }
            }

        } else if (S_ISREG(fileinfo.st_mode)) {
            if ((fileinfo.st_size == 0) && (m_flags & TraversalFlags::IgnoreEmptyFiles)) {
                close(direntfd);
                continue;
            }

            File child(name, fileinfo, dir);
            auto range = m_seenFiles.equal_range(child.size());
            if (range.first == m_seenFiles.end())
                range.first = range.second;
            auto match = std::any_of(range.first, range.second,
                [&child](std::pair<const off_t, std::unique_ptr<File>>& o) { return *(o.second) == child; });

            if (match) {
                //std::cerr << "Known " << child << std::endl;
            } else {
                //std::cerr << child << std::endl;
                m_seenFiles.emplace_hint(range.second,
                    child.size(), std::unique_ptr<File>(new File(child)));
            }

        } else if (S_ISLNK(fileinfo.st_mode)) {
            if (!(m_flags & TraversalFlags::FollowSymlinks)) {
                close(direntfd);
                continue;
            }

            auto it = m_seenLinks.find(std::pair<ino_t, dev_t>(fileinfo.st_ino, fileinfo.st_dev));
            if (it != m_seenLinks.end()) {
                // FIXME: add as alias
                std::cerr << "Known  S:" << dir->path() << name << std::endl;

            } else {
                std::string linktarget(fileinfo.st_size, '\0');
                auto len = readlinkat(direntfd, "", &linktarget[0], fileinfo.st_size + 1);

                auto child = new Symlink(name, fileinfo, dir);
                child->setTarget(std::move(linktarget));
                std::cerr << *child << std::endl;

                m_seenLinks.emplace( std::pair<ino_t, dev_t>(child->inode(), child->device()),
                    std::unique_ptr<const Symlink>(child));

                auto res = examineSymlink(dirfd, dir, linktarget);
                if (!res)
                    std::cerr << "Res: " << "(none)" << std::endl;
            }

        }
        close(direntfd);
    }

    closedir(dirstream);
    return 0;
}

const DirEnt*
FileFinder::examineSymlink(int dirfd, const Directory* parentDir, const std::string& target)
{
    // boost::string_ref?
    auto is_separator = [](const char& c){ return c == '/'; };
    auto tr = boost::trim_right_copy_if(target, is_separator);

    std::vector<std::string> components;
    boost::split(components, tr, is_separator, boost::token_compress_on);

    auto end = std::remove_if(components.begin(), components.end(),
        [](const std::string& s) {
            return s == ".";
        });
    components.erase(end, components.end());

    if (target[0] == '/') {
        components[0] = "/";
        parentDir = nullptr;
    }

    assert(components.size());

    return examinePathComponent(dirfd, parentDir, components);
}

const DirEnt*
FileFinder::examinePathComponent(int dirfd, const Directory* parentDir, std::vector<std::string> components)
{
    struct stat fileinfo;
    auto& c = components[0];
    int pathfd = openat(dirfd, c.c_str(), O_PATH | O_NOFOLLOW);
    if (pathfd < 0) {
        if (parentDir)
            std::cerr << "Cant open " << *parentDir << " " << c << std::endl;
        return nullptr;
    }
    fstatat(pathfd, "", &fileinfo, AT_EMPTY_PATH);

    if (S_ISDIR(fileinfo.st_mode)) {
        if (components.size() > 1) {
            auto child = Directory(c, fileinfo, parentDir);
            //std::cerr << "\tE:" << child << std::endl;
            std::vector<std::string> ncomponents(components.begin() + 1, components.end());
            auto res = examinePathComponent(pathfd, new Directory(child), ncomponents);
            close(pathfd);
            return res;
        } else {
            auto it = m_seenDirectories.find(std::pair<ino_t, dev_t>(fileinfo.st_ino, fileinfo.st_dev));
            if (it != m_seenDirectories.end()) {
                std::cerr << "Known " << *(it->second) << std::endl;
                close(pathfd);
                return (*it).second.get();
            }

            auto child = new Directory(c, fileinfo, parentDir);
            int err = traverseDirectory(pathfd, child);
            close(pathfd);
            return child;
        }

    } else if (S_ISREG(fileinfo.st_mode)) {
        if (components.size() > 1) {
            // error
        }
        File child(c, fileinfo, parentDir);
        std::cerr << child << std::endl;

        auto it = std::find_if(m_seenFiles.begin(), m_seenFiles.end(),
            [&child](std::pair<const off_t, std::unique_ptr<File>>& o) { return *(o.second) == child; });
        if (it != m_seenFiles.end()) {
            std::cerr << "Known " << *(it->second) << std::endl;
            // FIXME: add as alias
            close(pathfd);
            return (*it).second.get();
        }

        it = m_seenFiles.emplace(child.size(), std::unique_ptr<File>(new File(child)));
        close(pathfd);
        return (*it).second.get();

    } else if (S_ISLNK(fileinfo.st_mode)) {
        if (components.size() > 1) {
            // error
        }

        auto it = m_seenLinks.find(std::pair<ino_t, dev_t>(fileinfo.st_ino, fileinfo.st_dev));
        if (it != m_seenLinks.end()) {
            // FIXME: add as alias
            std::cerr << "Known " << *(it->second) << std::endl;
            close(pathfd);
            return (*it).second.get();
        }

        // otherwise, examineSymlink
        std::string linktarget(fileinfo.st_size, '\0');
        auto len = readlinkat(pathfd, "", &linktarget[0], fileinfo.st_size + 1);
        auto child = new Symlink(c, fileinfo, parentDir);
        child->setTarget(std::move(linktarget));
        std::cerr << *child << std::endl;
        m_seenLinks.emplace( std::pair<ino_t, dev_t>(child->inode(), child->device()),
            std::unique_ptr<const Symlink>(child));
        close(pathfd);

        auto res = examineSymlink(dirfd, parentDir, linktarget);
        return res;
    }

    std::cerr << "Ignoring special file" << std::endl;
    close(pathfd);
    return nullptr;
}

std::vector<std::unique_ptr<File>>
FileFinder::files()
{
    std::vector<std::unique_ptr<File>> files;
    files.reserve(m_seenFiles.size());

    for(auto& o :  m_seenFiles) {
        files.emplace_back(std::move(o.second));
    }

    std::cout << "Copied " << files.size() << std::endl;
    m_seenFiles.clear();
    return files;
}

int
FileFinder::findFiles(const char* path, TraversalFlags flags)
{
    return findFiles(std::string(path), flags);
}

int
FileFinder::findFiles(const std::string& path, TraversalFlags flags)
{
    m_flags = flags;

    examineSymlink(AT_FDCWD, 0, path);
}

} // namespace LibFDupe

// vim: set ai expandtab shiftwidth=4
