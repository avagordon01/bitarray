#include <cstring>
#include <array>
#include <vector>
#include <span>
#include <cstdint>
#include <limits>
#include <iostream>
#include <immintrin.h>
#include <bit>
#include <stdexcept>
#include <algorithm>

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
inline constexpr size_t match_underlying = -1;
template <
    size_t Bits = match_underlying,
    typename WordType = size_t,
    typename Container = std::array<WordType, 1 + (Bits - 1) / std::numeric_limits<WordType>::digits>
>
requires (std::numeric_limits<WordType>::is_integer && !std::numeric_limits<WordType>::is_signed)
struct bitarray {
    using self_type = bitarray<Bits, WordType, Container>;
    static constexpr size_t WordBits = std::numeric_limits<WordType>::digits;

    Container data {};
    size_t _size = 0;

    constexpr size_t infer_size() {
        if constexpr (Bits == match_underlying) {
            return data.size() * WordBits;
        } else {
            return Bits;
        }
    }
    constexpr size_t size() const {
        if constexpr (Bits == match_underlying) {
            return data.size() * WordBits;
        } else {
            return _size;
        }
    }
    void resize(size_t s) {
        if constexpr (Bits == match_underlying) {
            throw std::runtime_error{"trying to resize a bitarray with match_underlying = true"};
        } else {
            data.resize(s / WordBits);
            _size = s;
        }
    }

    bitarray(const self_type& other):
        //TODO add ctor for mismatching word types
        data{other.data},
        _size{infer_size()}
    {}
    self_type& operator=(const self_type& other) {
        data = other.data;
        _size = infer_size();
        return *this;
    }
    bitarray(Container c):
        data{c},
        _size{infer_size()}
    {}
    template<typename ...Ts>
    requires (std::integral<Ts> && ...)
    bitarray(Ts... ts):
        data{static_cast<WordType>(ts)...},
        _size{infer_size()}
    {}
    bitarray():
        data{},
        _size{infer_size()}
    {}

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
    void sanitize() {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshift-count-overflow"
        if (size() % WordBits != 0) {
            data.back() &= ones() >> (WordBits - size() % WordBits);
        }
#pragma GCC diagnostic pop
    }
public:
    bool all() const {
        if (size() % WordBits == 0) {
            for (size_t i = 0; i < data.size(); i++)
                if (data[i] != ones())
                    return false;
        } else {
            if (data.back() != ones() >> (WordBits - size() % WordBits))
                return false;
            for (size_t i = 0; i + 1 < data.size(); i++)
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
        for (size_t i = 0; i < data.size(); i++)
            if (data[i] != 0)
                return i * WordBits + std::countr_zero(data[i]);
        return size();
    }
    int countr_one() const {
        for (size_t i = 0; i < data.size(); i++)
            if (data[i] != ones())
                return i * WordBits + std::countr_one(data[i]);
        return size();
    }
    int countl_zero() const {
        for (size_t i = data.size(); i--;)
            if (data[i] != 0)
                return size() - i * WordBits - (WordBits - std::countl_zero(data[i]));
        return size();
    }
    int countl_one() {
        //XXX this doesn't seem like the best way
        return (~*this).countl_zero();
    }
    int bit_width() {
        return size() - countl_zero();
    }
    self_type bit_floor() {
        if (any())
            return self_type{1} << (bit_width() - 1);
        return 0;
    }
    self_type bit_ceil() {
        //TODO
    }
    void wordswap() {
        std::reverse(data.begin(), data.end());
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
    void set() {
        for (auto& x: data)
            x = ones();
        sanitize();
    }
    constexpr void set(size_t pos, bool value = true) {
        if (pos >= size()) {
            throw std::out_of_range{"set() called with pos " + std::to_string(pos) + " on bitset of size " + std::to_string(size())};
        }
        if (value) {
            data[pos / WordBits] |= one() << (pos % WordBits);
        } else {
            data[pos / WordBits] &= ~(one() << (pos % WordBits));
        }
    }
    void reset() {
        for (auto& x: data)
            x = zero();
    }
    void reset(size_t pos) {
        if (pos >= size()) {
            throw std::out_of_range{"reset() called with pos " + std::to_string(pos) + " on bitset of size " + std::to_string(size())};
        }
        data[pos / WordBits] &= ~(one() << (pos % WordBits));
    }
    void flip() {
        for (auto& x: data)
            x = ~x;
        sanitize();
    }
    void flip(size_t pos) {
        if (pos >= size()) {
            throw std::out_of_range{"flip() called with pos " + std::to_string(pos) + " on bitset of size " + std::to_string(size())};
        }
        data[pos / WordBits] ^= one() << (pos % WordBits);
    }
    void set_word_at_pos(WordType x, size_t pos) {
        if (pos >= size()) {
            throw std::out_of_range{"set_word_at_pos() called with pos " + std::to_string(pos) + " on bitset of size " + std::to_string(size())};
        }
        size_t offset = pos % WordBits;
        data[pos / WordBits] |= x << offset;
        if (offset != 0 && pos / WordBits + 1 < data.size()) {
            data[pos / WordBits + 1] |= x >> (WordBits - offset);
        }
    }
    WordType get_word_at_pos(size_t pos) const {
        if (pos >= size()) {
            throw std::out_of_range{"get_word_at_pos() called with pos " + std::to_string(pos) + " on bitset of size " + std::to_string(size())};
        }
        size_t offset = pos % WordBits;
        WordType out = data[pos / WordBits] >> offset;
        if (offset != 0 && pos / WordBits + 1 < data.size()) {
            out |= data[pos / WordBits + 1] << (WordBits - offset);
        }
        return out;
    }
    bool operator==(const self_type& rhs) const {
        for (size_t i = 0; i < data.size(); i++)
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
    self_type operator~() const {
        self_type v = *this;
        v.flip();
        return v;
    }
    void operator&=(const self_type& rhs) {
        for (size_t i = 0; i < data.size(); i++)
            data[i] &= rhs.data[i];
    }
    void operator|=(const self_type& rhs) {
        for (size_t i = 0; i < data.size(); i++)
            data[i] |= rhs.data[i];
    }
    void operator^=(const self_type& rhs) {
        for (size_t i = 0; i < data.size(); i++)
            data[i] ^= rhs.data[i];
        sanitize();
    }
    self_type operator&(const self_type& rhs) const {
        self_type x = *this;
        x &= rhs;
        return x;
    }
    self_type operator|(const self_type& rhs) const {
        self_type x = *this;
        x |= rhs;
        return x;
    }
    self_type operator^(const self_type& rhs) const {
        self_type x = *this;
        x ^= rhs;
        return x;
    }
    self_type operator<<(size_t shift) const {
        self_type x{};
        for (size_t pos = shift, i = 0; pos < size() && i < data.size(); pos += WordBits, i++) {
            x.set_word_at_pos(data[i], pos);
        }
        return x;
    }
    self_type operator<<=(size_t shift) {
        self_type x{};
        for (size_t pos = shift, i = 0; pos < size() && i < data.size(); pos += WordBits, i++) {
            x.set_word_at_pos(data[i], pos);
        }
        *this = x;
        return *this;
    }
    self_type operator>>(size_t shift) const {
        self_type x = *this;
        x >>= shift;
        return x;
    }
    self_type operator>>=(size_t shift) {
        size_t i = 0;
        for (size_t pos = shift; pos < size() && i < data.size(); pos += WordBits, i++) {
            data[i] = get_word_at_pos(pos);
        }
        for (; i < data.size(); i++) {
            data[i] = 0;
        }
        return *this;
    }
    self_type rotl(int shift) {
        if (shift < 0) {
            return rotr(-shift);
        }
        shift %= size();
        return (*this << shift) | (*this >> (size() - shift));
    }
    self_type rotr(int shift) {
        if (shift < 0) {
            return rotl(-shift);
        }
        shift %= size();
        return (*this >> shift) | (*this << (size() - shift));
    }



    template<size_t M, size_t O = M>
    bitarray<O, WordType> gather(bitarray<M, WordType> mask) {
        if (M > size()) {
            throw std::out_of_range{"gather operation mask length must be <= input length"};
        }
        bitarray<O, WordType> output {};
        for (size_t i = 0, pos = 0; i < mask.data.size(); i++) {
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
        if (M < size()) {
            throw std::out_of_range{"scatter operation mask length must be >= input length"};
        }
        bitarray<M, WordType> output {};
        for (size_t i = 0, pos = 0; i < output.data.size(); i++) {
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
};

template<typename T, size_t S>
bitarray(std::array<T, S>) -> bitarray<std::numeric_limits<T>::digits * S, T, std::array<T, S>>;
template<typename T>
bitarray(std::vector<T>) -> bitarray<match_underlying, T, std::vector<T>>;
template<typename T>
bitarray(std::span<T, std::dynamic_extent>) -> bitarray<match_underlying, T, std::span<T, std::dynamic_extent>>;
template<typename T, size_t S>
bitarray(std::span<T, S>) -> bitarray<std::numeric_limits<T>::digits * S, T, std::span<T, S>>;
}
