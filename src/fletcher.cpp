
/* calculate fletcher checksum
 *
 * Fletcher checksum can be implemented for different word sizes, e.g.
 * uint8_t (Fletcher16), uint16_t (Fletcher32) or uint32_t (Fletcher64).
 * The checksum has twice the size of the data word, which is generated
 * by concatenating a upper and a lower checksum.
 *
 * Formula for the partial checksums is as follows:
 *
 * \f[
 *  lower = ( \sum_{i=0}^{n-1} d[i] ) \mod m
 * \f]
 * \f[
 *  upper = ( \sum_{i=0}^{n-1} (n-i) * d[i] ) \mod m
 * \f]
 *
 * The modulus operation can be calculated either every summation step,
 * or after the summation. Care must be taken the acumulator does not
 * overflow if the modulus is delayed.
 *
 */

#include <iostream>
#include "fletcher.h"

namespace LibFDupe {

template <class Accu, class Word>
constexpr size_t
chunksize();

template <>
constexpr size_t
chunksize<uint64_t, uint32_t>()
{
    /* chunksize depends on the accumulator size and the wordsize
     * lower bound is reached if all data words have maximum value,
     * i.e. d is constant word::max.
     *
     * \f[
     *  upper <= \sum (n-i) * max(d)
     *   = max(d) * \sum (n-i)
     *   = max(d) * \sum (i+1)
     *   = max(d) * (n+1) * (n+2) / 2
     * \f]
     *
     * This is approximately 1.4 * sqrt(accu::max / word::max)
     */
     return 1024;
}

cChecksum64
fletcher64_chunk(const uint32_t* data, size_t words)
{
    uint64_t sum1 = 0;
    uint64_t sum2 = 0;

    auto chunkend = data + words;
    do {
        sum1 += *data;
        sum2 += sum1;
        data++;
    } while (data != chunkend);

    uint32_t s1 = sum1 % ((1ul<<32) - 1);
    uint32_t s2 = sum2 % ((1ul<<32) - 1);

    return { s1, s2, words };
}

cChecksum64
fletcher64(const uint32_t* data, size_t words)
{
    cChecksum64 chk = { 0, 0, 0 };
    const auto cs = chunksize<uint64_t, uint32_t>();

    while (words > cs) {
        /* split data into chunks to avoid overflow of
         * accumulators */
        chk << fletcher64_chunk(data, words);
        data += words;
        words -= cs;
    }

    if (words)
        chk << fletcher64_chunk(data, words);

    return chk;
}

} // namespace

