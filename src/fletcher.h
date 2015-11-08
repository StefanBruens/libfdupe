#include <cstddef>
#include <cstdint>

namespace LibFDupe {

class cChecksum64
{
public:
    bool operator==(const cChecksum64& other) {
        return (lower == other.lower) &&
               (upper == other.upper) &&
               (len == other.len);
    }

    cChecksum64& operator<<(const cChecksum64& other) {
        uint64_t tl = lower + other.lower;
        uint64_t tu = upper;
        tu += other.len * lower;
        tu += other.upper;
        lower = tl % ((1ul<<32) - 1);
        upper = tu % ((1ul<<32) - 1);
        len += other.len;
        len %= ((1ul<<32) - 1);
        return *this;
    }

    uint64_t uint64() const {
        uint64_t ret = upper;
        ret <<= 32;
        ret |= lower;
        return ret;
    }

    uint32_t lower;
    uint32_t upper;
    size_t len;
};

cChecksum64
fletcher64(const uint32_t* data, size_t words);

} // namespace
