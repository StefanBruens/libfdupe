#ifndef __FDUPE_FILES_H__
#define __FDUPE_FILES_H__

#include <string>
#include <vector>
#include <map>
#include <sys/stat.h>
#include <memory>

namespace LibFDupe {

class DirEnt
{
public:
    DirEnt(const std::string& name);
    DirEnt(const std::string& name, const struct stat& stat, const DirEnt* parent);
    friend std::ostream& operator<<(std::ostream& stream, const DirEnt& dirent);
    virtual ~DirEnt();
    virtual std::string path() const;
    const std::string& name() const { return m_name; }
    ino_t inode() const;
    dev_t device() const;

protected:
    virtual void dump(std::ostream& stream) const = 0;

    const DirEnt* m_parent;
    struct stat m_stat;
    std::string m_name;         //< path relative to parent
};

class Directory : public DirEnt
{
public:
    Directory(const std::string& name, const struct stat& stat, const DirEnt* parent = nullptr);
    virtual std::string path() const;
    bool operator==(const Directory& other) const;

private:
    virtual void dump(std::ostream& stream) const;
};

class File : public DirEnt
{
public:
    File(const std::string& name, const struct stat& stat, const DirEnt* parent);
    off_t size() const;

private:
    virtual void dump(std::ostream& stream) const;
};

class Symlink : public DirEnt
{
public:
    Symlink(const std::string& name, const struct stat& stat, const DirEnt* parent);
    void setTarget(std::string&& target);

private:
    virtual void dump(std::ostream& stream) const;
    std::string m_target;
};

} // namespace LibFDupe

#endif // __FDUPE_FILES_H__

