#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <iostream>
#include <immintrin.h>
#include <type_traits>

template<typename T>
std::pair<T, T> euclid_divide(T x, T y) {
    static_assert(std::is_integral<T>::value, "type parameter T to euclid_divide must be integral");
    static_assert(std::is_signed<T>::value, "type parameter T to euclid_divide must be signed");
    T q = x / y;
    T r = x % y;
    if (r < 0) {
        if (y >= 0) {
            r += y;
            q -= 1;
        } else {
            r -= y;
            q += 1;
        }
    }
    return {q, r};
}

template <size_t N, typename T = uint_fast32_t>
class bitarray {
    using self_type = bitarray<N, T>;
    static_assert(std::numeric_limits<T>::is_integer, "storage type must be an integer type");

public:
    static constexpr size_t BITS_PER_WORD = std::numeric_limits<T>::digits;
    static constexpr size_t WORDS = 1 + (N - 1) / BITS_PER_WORD;
    static_assert(WORDS * BITS_PER_WORD >= N, "oh no");

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
    static constexpr T ones() {
        return ~static_cast<T>(0);
    };
    void sanitize() {
        if constexpr (N % BITS_PER_WORD != 0) {
            size_t offset = N % BITS_PER_WORD;
            data.back() &= ~(ones() << offset);
        }
    };
public:
    bool all() const {
        //FIXME this is wrong for non word aligned N
        for (auto& x: data)
            if (x != ones())
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
            x = ones();
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
            x ^= ones();
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
    template<typename F, size_t M>
    static void map(const bitarray<N, T>& input, bitarray<M, T>& output, F f, ssize_t offset) {
        //FIXME the iteration needs to happen in the correct order to be able to happen in-place
        //i.e. reverse iterator for shift left and forward iterator for shift right
#pragma unroll
        for (size_t i = 0; i < input.WORDS; i++) {
            auto t = f(input.data[i]);
            ssize_t start = static_cast<ssize_t>(i * input.BITS_PER_WORD * t.size()) + offset;
            auto [start_word, start_bit] = euclid_divide<ssize_t>(start, static_cast<ssize_t>(output.BITS_PER_WORD));
#pragma unroll
            for (size_t j = start_word;
                    j - start_word < t.size() &&
                    j < output.data.size()
                ; j++) {
                output.data[j] |= t[j - start_word] << start_bit;
            }
            if (start_bit == 0)
                continue;
            start_word += 1;
#pragma unroll
            for (size_t j = start_word;
                    j - start_word < t.size() &&
                    j < output.data.size()
                ; j++) {
                output.data[j] |= t[j - start_word] >> (output.BITS_PER_WORD - start_bit);
            }
        };
    }
public:
    friend self_type operator<<(const self_type& lhs, size_t _shift) {
        auto f = [](T x) -> std::array<T, 1> { return {x}; };
        self_type x{};
        map(lhs, x, f, static_cast<ssize_t>(_shift));
        return x;
    };
    friend self_type operator>>(const self_type& lhs, size_t _shift) {
        auto f = [](T x) -> std::array<T, 1> { return {x}; };
        self_type x{};
        map(lhs, x, f, -static_cast<ssize_t>(_shift));
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
    //bitarray<N, T> interleave(std::array<bitarray<N / Step, T>, Step> inputs) {
    bitarray<N * Step, T> interleave(bitarray<N, T> input) {
        auto f = [](T x) -> std::array<T, Step> { return {
            _pdep_u64(x, short_mask<Step, 0>()),
            _pdep_u64(x >> std::numeric_limits<T>::digits / Step, short_mask<Step, 0>()),
        };};
        bitarray<N * Step, T> output{};
        map(input, output, f, 0);
        return output;
    };
    template <size_t Step>
    static std::array<bitarray<N / Step, T>, Step> deinterleave(bitarray<N, T> input) {
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
