
#include <algorithm>
#include <list>
#include <iostream>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cassert>

#include <filecompare.h>
#include <files.h>
#include "fletcher.h"
#include "prefixtree.h"

namespace LibFDupe {

HashFile::HashFile(HashFile&& o)
{
    // std::cout << "HashFile&& " << &o << " -> " << this << " [" << o.file << " " << o.fd << "]" << std::endl;
    file = o.file;
    mapaddr = o.mapaddr;
    fd = o.fd;

    o.fd = 0;
    o.mapaddr = nullptr;
    o.file = nullptr;
}

bool
HashFile::map()
{
    if (mapaddr)
        return true;
    // std::cout << "map() @" << this << " " << file << *file << std::endl;
    fd = open(file->path().c_str(), O_RDONLY);
    if (fd < 0)
        return false;
    mapaddr = reinterpret_cast<uint8_t*>(
        mmap(nullptr, size(), PROT_READ, MAP_PRIVATE, fd, 0));
    return true;
}

void
HashFile::unmap()
{
    // std::cout << "unmap() @" << this << " fd: " << fd << std::endl;
    if (!mapaddr)
        return;
    close(fd);
    munmap(mapaddr, size());
    fd = 0;
    mapaddr = nullptr;
}

uint64_t
HashFile::getHash(off_t offset, size_t len) const
{
    // std::cout << "getHash() @" << this << std::endl;
    auto startaddr = mapaddr + offset;

    if (offset + len > size()) {
        len = size() - offset;
        auto rem = len % 4;
        len -= rem;
        auto chk = fletcher64(reinterpret_cast<uint32_t*>(startaddr), len/4);
        if (rem) {
            startaddr += len;
            uint8_t data[4] = { 0 };
            for (; rem > 0; rem--) {
                data[3-rem] = *(startaddr++);
            }
            chk << fletcher64(reinterpret_cast<uint32_t*>(data), 1);
        }

        return 1 + chk.uint64();
    }
    auto chk = fletcher64(reinterpret_cast<uint32_t*>(startaddr), len/4);

    return 1 + chk.uint64();
}

inline bool
HashFile::equalData(const HashFile& o, off_t offset, size_t len) const
{
    auto startaddr = mapaddr + offset;
    auto startaddr_o = o.mapaddr + offset;

    if (offset + len > size())
        len = size() - offset;

    return memcmp(startaddr, startaddr_o, len) == 0;
}

template<>
void
PrefixTreeNode::insertRight(const HashFile& left, HashFile&& right, off_t offset)
{
    if (left.equalData(right, offset, 4096)) {
        // std::cout << "IR @" << offset << ": " << *left.file << " == " << *right.file << std::endl;
        offset += 4096;
        if (offset >= left.size()) {
            std::cout << "IR " << left.size() << " (" << files.size() << "): " << *left.file << " == " << *right.file;// << std::endl;
            std::cout << " : " << tree.nodes.size() << " " << files.size() << std::endl;
            right.unmap();
            if (tree.nodes.empty()) {
                assert(files.size() > 0);
                files.emplace_back(std::move(right));
            } else {
                assert(tree.nodes[0].files.size() > 0);
                tree.nodes[0].files.emplace_back(std::move(right));
            }

        } else {
            if (tree.nodes.empty()) {
                tree.nodes.emplace_back(0);
                //std::cout << "N @" << offset << ": " << files.size() << std::endl;
                std::swap(files, tree.nodes[0].files);
            }
            tree.nodes[0].insertRight(
                left, std::move(right), offset);
        }
    } else {
        if (tree.nodes.empty()) {
            tree.nodes.emplace_back(0);
            //std::cout << "N2 @" << offset << ": " << files.size() << std::endl;
            std::swap(files, tree.nodes[0].files);
        }
        if (tree.nodes[0].hashValue == 0) {
            tree.nodes[0].hashValue = left.getHash(offset, 4096);
        }
        std::cout << "IR split [" << tree.nodes[0].hashValue << "] @" << offset << ": " << *left.file << " | " << *right.file << std::endl;
        tree.insertFile(std::move(right), offset);
    }
}

void
PrefixTree::insertFile(HashFile&& file, off_t offset)
{
    if (nodes.empty()) {
        std::cout << "I  " << file.size() << ": " << *file.file << std::endl;
        nodes.emplace_back(0, std::move(file));
        return;
    }

    if (!file.map())
           return;

    uint64_t hash = 0;
    if (nodes.size() > 1) {
        hash = file.getHash(offset, 4096);
    } else {
        assert(nodes.empty() || (nodes[0].hashValue == 0));
    }

    auto first = std::find_if(nodes.begin(), nodes.end(),
            [hash](const PrefixTreeNode& n) { return hash <= n.hashValue; });
    auto last = std::find_if(first, nodes.end(),
        [hash](const PrefixTreeNode& n) { return hash < n.hashValue; });

    auto size = last-first;
    if (size > 1) {
        std::cerr << *(file.file) << " collision: " << hash << std::endl;
    }
    auto match = std::find_if(first, last, [&file, offset](PrefixTreeNode& n)
    {
        auto& o = n.firstFile();
        o.map();

        if (o.equalData(file, offset, 4096)) {
            // std:: cout << file.file << " == " << o.file << " (" << offset << ")" << std::endl;
            return true;

        } else {
            if (n.hashValue == 0) {
                n.hashValue = o.getHash(offset, 4096);
                //std::cout << "UP split [" << n.hashValue << "] @" << offset << ": " << *file.file << " <> " << *o.file << std::endl;
                //std:: cout << file.file << " <> " << o.file << " (" << offset << ")" << std::endl;
            }
            o.unmap();
            return false;
        }
    });

    if (match == last) {
        if (nodes.size() > 0) {
            hash = file.getHash(offset, 4096);
            first = std::find_if(nodes.begin(), nodes.end(),
                [hash](const PrefixTreeNode& n) { return hash <= n.hashValue; });
        }
        std::cout << "A  " << file.size() << " [" << hash << "]: " << *file.file << std::endl;
        file.unmap();
        nodes.emplace(first, hash, std::move(file));

    } else {
        offset += 4096;
        if (offset >= file.size()) {
            // both files are identical
            std::cout << "i  " << file.size() << *(*match).firstFile().file << " == " << *file.file << std::endl;
            (*match).firstFile().unmap();
            file.unmap();
            (*match).files.push_back(std::move(file));

        } else {
            auto& left = (*match).firstFile();
            //std:: cout << &file << " ? " << &left << std::endl;
            (*match).insertRight(left, std::move(file), offset);
            (*match).firstFile().unmap();

        }

    }
}

template<class HashType>
HashFile&
PrefixTreeNodeBase<HashType>::firstFile()
{
    if (files.empty())
        return tree.nodes[0].firstFile();
    return files[0];
}

} // namespace
