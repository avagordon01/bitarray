#include <cstdint>
#include <limits>
#include <iostream>
#include <immintrin.h>
#include <bit>
#include <stdexcept>
#include <algorithm>
#include <optional>
#include <array>
#include <vector>

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

namespace bitarray {
template <size_t fixed_size = 0, typename T = std::array<size_t, fixed_size / std::numeric_limits<size_t>::digits>>
struct bitarray {
    using self_type = bitarray<fixed_size, T>;
    using WordType = typename T::value_type;
    static constexpr auto WordBits = std::numeric_limits<WordType>::digits;

    T data;
    //_size is for if the number of bits used is less than the underlying data has
    std::optional<size_t> _size = std::nullopt;

    bitarray(): data{}, _size(fixed_size) {}

    template<typename S>
    bitarray(S size): data{}, _size(size) {}

    bitarray(T& _data): data(_data), _size(std::nullopt) {}
    bitarray(T& _data, size_t s): data(_data), _size({s}) {}

    bitarray(T&& _data): data(std::move(_data)), _size(std::nullopt) {}
    bitarray(T&& _data, size_t s): data(_data), _size({s}) {}

    template <class CharT, class Traits>
    friend std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const self_type& x) {
        int base_digits = 0;
        if (os.flags() & std::ios_base::dec) {
            base_digits = 1; //XXX this is a hack, rather than default to decimal, we default to binary
            os << "0b";
        } else if (os.flags() & std::ios_base::oct) {
            base_digits = 3;
            os << "0o";
        } else if (os.flags() & std::ios_base::hex) {
            base_digits = 4;
            os << "0x";
        }
        for (ssize_t i = x.size(); i -= base_digits;) {
            unsigned char digit = 0;
            for (int j = 0; j < base_digits; j++) {
                digit <<= 1;
                digit += x[i + j];
            }
            os << std::to_string(digit);
        }
        return os;
    }

private:
    constexpr WordType zero() const {
        return static_cast<WordType>(0);
    }
    constexpr WordType one() const {
        return static_cast<WordType>(1);
    }
    constexpr WordType ones() const {
        return ~static_cast<WordType>(0);
    }
    WordType& back() {
        return *std::prev(std::end(data));
    }
    void sanitize() {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshift-count-overflow"
        if (size() % WordBits != 0) {
            back() &= ones() >> (WordBits - size() % WordBits);
        }
#pragma GCC diagnostic pop
    }
public:
    constexpr size_t size() const {
#ifndef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
        return _size.value_or(std::size(data) * WordBits);
#ifndef __clang__
#pragma GCC diagnostic pop
#endif
    }
    void resize(size_t s) {
        if (s / WordBits >= std::size(data)) {
            data.resize(s / WordBits);
            _size = s;
        } else {
            _size = {s};
        }
    }
    bool all() const {
        if (size() % WordBits == 0) {
            for (size_t i = 0; i < std::size(data); i++)
                if (data[i] != ones())
                    return false;
        } else {
            if (back() != ones() >> (WordBits - size() % WordBits))
                return false;
            for (size_t i = 0; i + 1 < std::size(data); i++)
                if (data[i] != ones())
                    return false;
        }
        return true;
    }
    bool any() const {
        for (auto& x: data)
            if (x != 0)
                return true;
        return false;
    }
    bool none() const {
        for (auto& x: data)
            if (x != 0)
                return false;
        return true;
    }
    int count() const {
        int count = 0;
        for (auto& x: data)
            if constexpr (sizeof(WordType) <= 8) {
                count += std::popcount(x);
            } else if (sizeof(WordType) <= 16) {
                count += std::popcount(static_cast<uint64_t>(x >> 64)) + std::popcount(static_cast<uint64_t>(x));
            }
        return count;
    }
    bool has_single_bit() const {
        return count() == 1;
    }
    int countr_zero() const {
        for (size_t i = 0; i < std::size(data); i++)
            if (data[i] != 0)
                return i * WordBits + std::countr_zero(data[i]);
        return size();
    }
    int countr_one() const {
        for (size_t i = 0; i < std::size(data); i++)
            if (data[i] != ones())
                return i * WordBits + std::countr_one(data[i]);
        return size();
    }
    int countl_zero() const {
        for (size_t i = std::size(data); i--;)
            if (data[i] != 0)
                return size() - i * WordBits - (WordBits - std::countl_zero(data[i]));
        return size();
    }
    int countl_one() const {
        //FIXME implement without a temporary
        //return (~*this).countl_zero();
        return 0;
    }
    int bit_width() const {
        return size() - countl_zero();
    }
    self_type bit_floor() const {
        int w = bit_width();
        reset();
        if (w != 0) {
            set(w - 1);
        }
        return *this;
    }
    self_type bit_ceil() const {
        //TODO
    }





    void wordswap() {
        std::reverse(std::begin(data), std::end(data));
    }
    void byteswap() {
        wordswap();
        for (auto& x: data) {
            //std::byteswap(x);
        }
    }
    void bitswap() {
        byteswap();
        //TODO bitswap
    }
    self_type set() {
        for (auto& x: data)
            x = ones();
        sanitize();
        return *this;
    }
    constexpr self_type set(size_t pos, bool value = true) {
        if (pos >= size()) {
            throw std::out_of_range{"set() called with pos " + std::to_string(pos) + " on bitset of size " + std::to_string(size())};
        }
        if (value) {
            data[pos / WordBits] |= one() << (pos % WordBits);
        } else {
            data[pos / WordBits] &= ~(one() << (pos % WordBits));
        }
        return *this;
    }
    self_type reset() {
        for (auto& x: data)
            x = zero();
        return *this;
    }
    self_type reset(size_t pos) {
        if (pos >= size()) {
            throw std::out_of_range{"reset() called with pos " + std::to_string(pos) + " on bitset of size " + std::to_string(size())};
        }
        data[pos / WordBits] &= ~(one() << (pos % WordBits));
        return *this;
    }
    self_type flip() {
        for (auto& x: data)
            x = ~x;
        sanitize();
        return *this;
    }
    self_type flip(size_t pos) {
        if (pos >= size()) {
            throw std::out_of_range{"flip() called with pos " + std::to_string(pos) + " on bitset of size " + std::to_string(size())};
        }
        data[pos / WordBits] ^= one() << (pos % WordBits);
        return *this;
    }
    void set_word_at_pos(WordType x, size_t pos) {
        if (pos >= size()) {
            throw std::out_of_range{"set_word_at_pos() called with pos " + std::to_string(pos) + " on bitset of size " + std::to_string(size())};
        }
        size_t offset = pos % WordBits;
        data[pos / WordBits] |= x << offset;
        if (offset != 0 && pos / WordBits + 1 < std::size(data)) {
            data[pos / WordBits + 1] |= x >> (WordBits - offset);
        }
    }
    WordType get_word_at_pos(size_t pos) const {
        if (pos >= size()) {
            throw std::out_of_range{"get_word_at_pos() called with pos " + std::to_string(pos) + " on bitset of size " + std::to_string(size())};
        }
        size_t offset = pos % WordBits;
        WordType out = data[pos / WordBits] >> offset;
        if (offset != 0 && pos / WordBits + 1 < std::size(data)) {
            out |= data[pos / WordBits + 1] << (WordBits - offset);
        }
        return out;
    }
    bool operator==(const self_type& rhs) const {
        for (size_t i = 0; i < std::size(data); i++)
            if (data[i] != rhs.data[i])
                return false;
        return true;
    }
    bool operator!=(const self_type& rhs) const {
        return !(*this == rhs);
    }
    constexpr bool at(size_t pos) const {
        if (pos >= size()) {
            throw std::out_of_range{"at() called with pos " + std::to_string(pos) + " on bitset of size " + std::to_string(size())};
        }
        return *this[pos];
    }
    constexpr bool operator[](size_t pos) const {
        return static_cast<bool>((data[pos / WordBits] >> (pos % WordBits)) & 1);
    }
    //TODO
    //for view/non-owning types (span)
    //operator& etc shouldn't compile (versus in-place operator &= which should be fine)

    //TODO
    //performance warning on using copy operators for large vectors

    self_type operator&=(const self_type& rhs) {
        //FIXME what if the dynamic sizes are different?
        for (size_t i = 0; i < std::size(data); i++)
            data[i] &= rhs.data[i];
        return *this;
    }
    self_type operator|=(const self_type& rhs) {
        //FIXME what if the dynamic sizes are different?
        for (size_t i = 0; i < std::size(data); i++)
            data[i] |= rhs.data[i];
        return *this;
    }
    self_type operator^=(const self_type& rhs) {
        //FIXME what if the dynamic sizes are different?
        for (size_t i = 0; i < std::size(data); i++)
            data[i] ^= rhs.data[i];
        sanitize();
        return *this;
    }
    self_type operator<<=(size_t shift) {
        for (size_t i = std::size(data); i--;) {
            set_word_at_pos(data[i], i * WordBits + shift);
        }
        return *this;
    }
    self_type operator>>=(size_t shift) {
        for (size_t i = 0; i < std::size(data); i++) {
            if (shift + i * WordBits < size()) {
                data[i] = get_word_at_pos(shift + i * WordBits);
            } else {
                data[i] = 0;
            }
        }
        return *this;
    }
    self_type rotl(int shift) {
        if (shift < 0) {
            return rotr(-shift);
        }
        shift %= size();
        //FIXME implement without temporaries
        //return (*this << shift) | (*this >> (size() - shift));
        return *this;
    }
    self_type rotr(int shift) {
        if (shift < 0) {
            return rotl(-shift);
        }
        shift %= size();
        //FIXME implement without temporaries
        //return (*this >> shift) | (*this << (size() - shift));
        return *this;
    }



    /*
    FIXME
    template<size_t M, size_t O = M>
    bitarray<O, WordType> gather(bitarray<M, WordType> mask) {
        static_assert(M <= size(), "gather operation mask length must be <= input length");
        bitarray<O, WordType> output {};
        for (size_t i = 0, pos = 0; i < mask.std::size(data); i++) {
            output.set_word_at_pos(pext(
                data[i],
                mask.data[i]
            ), pos);
            pos += std::popcount(mask.data[i]);
        }
        return output;
    }
    template<size_t M>
    bitarray<M, WordType> scatter(bitarray<M, WordType> mask) {
        static_assert(M >= size(), "scatter operation mask length must be >= input length");
        bitarray<M, WordType> output {};
        for (size_t i = 0, pos = 0; i < output.std::size(data); i++) {
            output.data[i] = pdep(
                get_word_at_pos(pos),
                mask.data[i]
            );
            pos += std::popcount(mask.data[i]);
        }
        return output;
    }


    template<size_t Len, size_t Num>
    static constexpr std::array<bitarray<Len>, Num> interleave_masks() {
        std::array<bitarray<Len>, Num> x{};
        for (size_t i = 0; i < Num; i++) {
            for (size_t j = i; j < Len; j += Num) {
                x[i].set(j);
            }
        }
        return x;
    }

    template<size_t Len, size_t Num>
    static bitarray<Len * Num> interleave(std::array<bitarray<Len>, Num> input) {
        bitarray<Len * Num> output {};
        std::array<bitarray<Len * Num>, Num> masks = interleave_masks<Len * Num, Num>();
        for (size_t j = 0; j < input.size(); j++) {
            output |= input[j].template scatter<Len * Num>(masks[j]);
        }
        return output;
    }
    template<size_t Len, size_t Num>
    static std::array<bitarray<Len>, Num> deinterleave(bitarray<Len * Num> input) {
        std::array<bitarray<Len>, Num> output {};
        std::array<bitarray<Len * Num>, Num> masks = interleave_masks<Len * Num, Num>();
        for (size_t j = 0; j < output.size(); j++) {
            output[j] = input.template gather<Len * Num, Len>(masks[j]);
        }
        return output;
    }
    */
};

template<std::integral S>
bitarray(S size) -> bitarray<0, std::vector<size_t>>;

template<typename T>
requires (!std::integral<T>)
bitarray(T data) -> bitarray<0, T>;
}
