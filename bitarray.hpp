#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <iostream>
#include <immintrin.h>

template <size_t N, typename T = uint_fast32_t>
class bitarray {
    using self_type = bitarray<N, T>;
    static_assert(std::numeric_limits<T>::is_integer, "storage type must be an integer type");

    static constexpr size_t BITS_PER_WORD = std::numeric_limits<T>::digits;
    static constexpr size_t WORDS = 1 + (N - 1) / BITS_PER_WORD;
    static_assert(WORDS * BITS_PER_WORD >= N, "oh no");

public:
    std::array<T, WORDS> data;

    bitarray() {
        data.fill(zero());
    }
    bitarray(std::initializer_list<T> list) {
        std::uninitialized_copy(list.begin(), list.begin() + WORDS, data.begin());
    }
private:
    static constexpr T zero() {
        return static_cast<T>(0);
    };
    static constexpr T one() {
        return static_cast<T>(1);
    };
    void sanitize() {
        if constexpr (N % BITS_PER_WORD != 0) {
            size_t offset = N % BITS_PER_WORD;
            data.back() &= ~((~zero()) << offset);
        }
    };
public:
    bool all() const {
        //FIXME this is wrong for non word aligned N
        for (auto& x: data)
            if (x != ~zero())
                return false;
        return true;
    };
    bool any() const {
        for (auto& x: data)
            if (x != 0)
                return true;
        return false;
    };
    bool none() const {
        for (auto& x: data)
            if (x != 0)
                return false;
        return true;
    };
    size_t count() const {
        size_t count = 0;
        for (auto& x: data)
            count += __builtin_popcountll(x);
        return count;
    };
    size_t size() const {
        return N;
    };
    size_t count_trailing_zeros() const {
        for (size_t i = 0; i < WORDS; i++) {
            if (data[i] != 0)
                return i * BITS_PER_WORD + __builtin_ctzll(data[i]);
        }
        return std::numeric_limits<T>::max();
    };
    size_t count_leading_zeros() const {
        //FIXME this is wrong for non word aligned N
        for (size_t i = WORDS; i--;) {
            if (data[i] != 0)
                return i * BITS_PER_WORD + __builtin_clzll(data[i]);
        }
        return std::numeric_limits<T>::max();
    };
    friend bool operator==(const self_type& lhs, const self_type& rhs) {
        for (size_t i = 0; i < WORDS; i++)
            if (lhs.data[i] != rhs.data[i])
                return false;
        return true;
    };
    friend bool operator!=(const self_type& lhs, const self_type& rhs) {
        return !(lhs == rhs);
    };
    constexpr bool operator[](size_t pos) const {
        return (data[pos / BITS_PER_WORD] >> (pos % BITS_PER_WORD)) & 1;
    };
    void set() {
        for (auto& x: data)
            x = ~zero();
        sanitize();
    };
    constexpr void set(size_t pos, bool value = true) {
        if (value) {
            data[pos / BITS_PER_WORD] |= one() << (pos % BITS_PER_WORD);
        } else {
            data[pos / BITS_PER_WORD] &= ~(one() << (pos % BITS_PER_WORD));
        }
    };
    void reset() {
        for (auto& x: data)
            x = zero();
    };
    void reset(size_t pos) {
        data[pos / BITS_PER_WORD] &= ~(one() << (pos % BITS_PER_WORD));
    };
    void flip() {
        for (auto& x: data)
            x ^= ~zero();
        sanitize();
    };
    void flip(size_t pos) {
        data[pos / BITS_PER_WORD] ^= one() << (pos % BITS_PER_WORD);
    };
    friend void operator~(self_type& lhs) {
        for (auto& x: lhs.data)
            x = ~x;
        lhs.sanitize();
    };
    friend void operator&=(self_type& lhs, const self_type& rhs) {
        for (size_t i = 0; i < WORDS; i++)
            lhs.data[i] &= rhs.data[i];
    };
    friend void operator|=(self_type& lhs, const self_type& rhs) {
        for (size_t i = 0; i < WORDS; i++)
            lhs.data[i] |= rhs.data[i];
    };
    friend void operator^=(self_type& lhs, const self_type& rhs) {
        for (size_t i = 0; i < WORDS; i++)
            lhs.data[i] ^= rhs.data[i];
    };
    friend self_type operator&(self_type& lhs, const self_type& rhs) {
        self_type x = lhs;
        x &= rhs;
        return x;
    };
    friend self_type operator|(self_type& lhs, const self_type& rhs) {
        self_type x = lhs;
        x |= rhs;
        return x;
    };
    friend self_type operator^(self_type& lhs, const self_type& rhs) {
        self_type x = lhs;
        x ^= rhs;
        return x;
    };
public:
    static void map(const bitarray<N, T>& input, bitarray<N, T>& output, T (*f)(T), ssize_t (*g)(ssize_t)) {
        //FIXME the iteration needs to happen in the correct order to be able to happen in-place
        //i.e. reverse iterator for shift left and forward iterator for shift right
        for (size_t i = 0; i < input.WORDS; i++) {
            //TODO f should return a compile-time fixed number of words per input word
            T t = f(input.data[i]);
            ssize_t start = g(static_cast<ssize_t>(i * input.BITS_PER_WORD));
            ssize_t end = g(static_cast<ssize_t>((i + 1) * input.BITS_PER_WORD - 1));
            ssize_t start_bit = start % output.BITS_PER_WORD;
            ssize_t start_word = start / output.BITS_PER_WORD;
            //size_t end_bit = end % output.BITS_PER_WORD;
            ssize_t end_word = end / output.BITS_PER_WORD;
            ssize_t bit_offset = start_bit;
#pragma clang loop unroll(full)
            for (ssize_t j = start_word; j <= end_word && j < static_cast<ssize_t>(output.WORDS); j++) {
                //FIXME this doesn't work with a "negative"/right shift
                if (bit_offset >= 0) {
                    output.data[j] |= t << bit_offset;
                } else {
                    output.data[j] |= t >> -bit_offset;
                }
                //TODO add % BITS_PER_WORD, when there can be more than two output words generated per input word
                bit_offset -= static_cast<ssize_t>(output.BITS_PER_WORD);
            }
        };
    }
public:
    friend self_type operator<<(self_type lhs, size_t _shift) {
        (void)_shift;
        auto f = [](T x) -> T { return x; };
        auto g = [](ssize_t offset) -> ssize_t { return offset - 6; };
        self_type x{};
        map(lhs, x, f, g);
        return x;
    };
    void operator>>=(size_t shift) {
        size_t i;
        for (i = 0; i + shift / BITS_PER_WORD < WORDS; i++) {
            data[i] = data[i + shift / BITS_PER_WORD] >> (shift % BITS_PER_WORD);
            if (i + 1 + shift / BITS_PER_WORD < WORDS)
                data[i] |= data[i + 1 + shift / BITS_PER_WORD] << (BITS_PER_WORD - shift % BITS_PER_WORD);
        }
        for (; i < WORDS; i++) {
            data[i] = 0;
        }
    };
    friend self_type operator>>(self_type&lhs, size_t shift) {
        self_type x = lhs;
        x >>= shift;
        return x;
    };
    template <size_t Step, size_t Start>
    static constexpr self_type mask() {
        self_type x{};
        for (size_t i = Start; i < N; i += Step) {
            x.set(i);
        }
        return x;
    };
    template <size_t Step, size_t Start>
    static constexpr T short_mask() {
        T x{0};
        for (size_t i = Start; i < BITS_PER_WORD; i += Step) {
            x |= one() << i;
        }
        return x;
    };
    template <size_t Step>
    bitarray<N, T> interleave(std::array<bitarray<N / Step, T>, Step> inputs) {
        bitarray<N, T> output{};
        for (auto& input: inputs) {
            for (auto& x: input.data) {
            }
        }
        return output;
    };
    template <size_t Step>
    std::array<bitarray<N / Step, T>, Step> deinterleave(bitarray<N, T> input) {
        std::array<bitarray<N / Step, T>, Step> outputs{};
        for (auto& x: input.data) {
        }
        return outputs;
    };
    template <class CharT, class Traits>
    friend std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const self_type& x) {
        for (ssize_t i = N; i--;) {
            bool bit = x[i];
            os << (bit ? '1' : '0');
        }
        return os;
    };
};
