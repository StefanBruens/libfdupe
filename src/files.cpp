#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <files.h>

namespace LibFDupe {

DirEnt::DirEnt(const std::string& name, const struct stat& stat, const DirEnt* parent)
    : m_parent(parent)
    , m_stat(stat)
    , m_name(name)
{
}

DirEnt::~DirEnt() {}

Directory::Directory(const std::string& name, const struct stat& stat, const DirEnt* parent)
    : DirEnt(std::string(), stat, parent)
{
    if (parent) {
        m_name = m_parent->name() + name + "/";
    } else {
        m_name = name;
        if (m_name.back() != '/')
            m_name.push_back('/');
    }
}

File::File(const std::string& name, const struct stat& stat, const DirEnt* parent)
    : DirEnt(name, stat, parent)
{
}

Symlink::Symlink(const std::string& name, const struct stat& stat, const DirEnt* parent)
    : DirEnt(name, stat, parent)
{
}

std::string
DirEnt::path() const
{
    if (m_parent)
         return m_parent->name() + m_name;

    return m_name;
}

ino_t DirEnt::inode() const
{
    return m_stat.st_ino;
}

dev_t DirEnt::device() const
{
    return m_stat.st_dev;
}

std::string
Directory::path() const
{
    return m_name;
}

std::ostream& operator<<(std::ostream& stream, const DirEnt& dirent)
{
    dirent.dump(stream);
    return stream;
}

void Symlink::setTarget(std::string&& target)
{
    m_target = target;
}

off_t File::size() const
{
    return m_stat.st_size;
}

int File::open() const
{
    std::string p;
    p.reserve(m_parent->name().size() + m_name.size());
    p += m_parent->name();
    p += m_name;
    return ::open(p.c_str(), O_RDONLY);
}

inline
void File::dump(std::ostream& stream) const
{
    stream << " F: ";
    if (m_parent)
         stream << m_parent->name();
    stream << m_name;
}

inline
void Directory::dump(std::ostream& stream) const
{
    stream << " D:" << name();
}

inline
void Symlink::dump(std::ostream& stream) const
{
    stream << " S: " << path() << " -> '" << m_target << "'";
}


} // namespace LibFDupe

// vim: set ai expandtab shiftwidth=4
