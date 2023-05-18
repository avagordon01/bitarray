#include <cstdint>
#include <limits>
#include <iostream>
#include <bit>
#include <stdexcept>
#include <algorithm>
#include <optional>
#include <array>
#include <vector>
#include <span>

#include "pdep-pext.hh"

namespace {
    template<typename WordType>
    constexpr size_t words_needed(size_t bits) {
        size_t WordBits = std::numeric_limits<WordType>::digits;
        return (bits + WordBits - 1) / WordBits;
    }
    template<typename Container>
    constexpr size_t bits_in_container(Container data) {
        return data.size() * std::numeric_limits<typename Container::value_type>::digits;
    }
}

namespace bitarray {
template <size_t Bits, typename WordType = size_t>
struct bitarray {
    using self_type = bitarray<Bits, WordType>;

    size_t _size = Bits;
    using DataType = std::array<WordType, words_needed<WordType>(Bits)>;
    DataType data{};

    bitarray() {
        sanitize();
    }
    bitarray(std::initializer_list<WordType> l) {
        std::copy(l.begin(), l.begin() + std::min(l.size(), data.size()), data.begin());
        sanitize();
    }
    bitarray(size_t size): _size(size), data() {
        if (size > Bits) {
            throw std::runtime_error("bitarray<x>(y): y > x, use dynamically resizable bitvector instead");
        }
        sanitize();
    }
    bitarray(size_t size, std::initializer_list<WordType> l) {
        if (size > Bits) {
            throw std::runtime_error("bitarray<x>(y): y > x, use dynamically resizable bitvector instead");
        }
        std::copy(l.begin(), l.begin() + std::min(l.size(), data.size()), data.begin());
        sanitize();
    }

#include "bitarray-impl.hh"
//XXX doing this include hack because partial template specialisation doesn't work on template aliases (`using`)
};

template<typename WordType = size_t>
struct bitvector {
    using self_type = bitvector<WordType>;

    size_t _size = std::dynamic_extent;
    std::vector<WordType> data{};

    bitvector() {}
    bitvector(std::vector<WordType> v): data(v) {
        _size = bits_in_container(data);
    }
    bitvector(size_t size): _size(size) {
        resize(_size);
    }
    bitvector(size_t size, std::vector<WordType> v): _size(size), data(v) {
        sanitize();
    }

#include "bitarray-impl.hh"
};

template<size_t Bits = std::dynamic_extent, typename WordType = size_t>
struct bitspan {
    using self_type = bitspan<Bits, WordType>;

    size_t _size;
    std::span<WordType, Bits> data{};

    bitspan(std::span<WordType, Bits> s): _size(Bits), data(s) {}
    bitspan(size_t size, std::span<WordType, Bits> s): _size(size), data(s) {
        sanitize();
    }

#include "bitarray-impl.hh"
};
}
