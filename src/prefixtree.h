
#include <unistd.h>
#include <vector>
#include <files.h>

namespace LibFDupe {

class HashFile
{
public:
    HashFile(HashFile&&);
    HashFile(const HashFile&) = delete;
    HashFile(File* file) : file(file) {};
    ~HashFile() { unmap(); }

    ino_t inode() const { return file->inode(); }
    dev_t device() const { return file->device(); }
    off_t size() const { return file->size(); }

    bool map();
    void unmap();

    uint64_t getHash(off_t offset, size_t len) const;
    bool equalData(const HashFile& o, off_t, size_t) const;

    File* file = nullptr;
    uint8_t* mapaddr = nullptr;
    int fd = 0;
private:
};

template <class HashType> class PrefixTreeNodeBase;
typedef PrefixTreeNodeBase<uint64_t> PrefixTreeNode;
class PrefixTree
{
public:
    void insertFile(HashFile&& file, off_t offset);

    std::vector<PrefixTreeNode> nodes;
private:
};

template <class HashType>
class PrefixTreeNodeBase
{
public:
    PrefixTreeNodeBase(HashType hash) : hashValue(hash) {};
    PrefixTreeNodeBase(HashType hash, HashFile&& file) : hashValue(hash)
    {
        files.reserve(2);
        files.emplace_back(std::move(file));
    }

    HashFile& firstFile();

    void insertRight(const HashFile& left, HashFile&& right, off_t offset);

    HashType hashValue;
    PrefixTree tree;
    std::vector<HashFile> files;
};

} // namespace
