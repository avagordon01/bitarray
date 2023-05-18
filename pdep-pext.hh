#include <cstdint>
#include <immintrin.h>
#include <bit>

namespace {
template<typename T>
T pext(T data, T mask) {
    if constexpr (sizeof(T) <= 4) {
        return _pext_u32(data, mask);
    } else if (sizeof(T) <= 8) {
        return _pext_u64(data, mask);
    } else if (sizeof(T) <= 16) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshift-count-overflow"
        T out0 = _pext_u64(data, mask);
        T out1 = _pext_u64(data >> 64, mask >> 64);
        return out0 | (out1 << std::popcount(static_cast<uint64_t>(mask)));
#pragma GCC diagnostic pop
    }
}

template<typename T>
T pdep(T data, T mask) {
    if constexpr (sizeof(T) <= 4) {
        return _pdep_u32(data, mask);
    } else if (sizeof(T) <= 8) {
        return _pdep_u64(data, mask);
    } else if (sizeof(T) <= 16) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshift-count-overflow"
        T out0 = _pdep_u64(data, mask);
        T out1 = _pdep_u64(data >> std::popcount(static_cast<uint64_t>(mask)), mask >> 64);
        return out0 | (out1 << 64);
#pragma GCC diagnostic pop
    }
}
}
