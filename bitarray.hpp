#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <iostream>

template <size_t N, typename T = uint_fast32_t>
class bitarray {
    using self_type = bitarray<N, T>;
    static_assert(std::numeric_limits<T>::is_integer, "storage type must be an integer type");

    static constexpr size_t BITS_PER_CHUNK = std::numeric_limits<T>::digits;
    static constexpr size_t CHUNKS = 1 + (N - 1) / BITS_PER_CHUNK;

    std::array<T, CHUNKS> data;
public:

    bitarray(std::initializer_list<T> list) {
        std::uninitialized_copy(list.begin(), list.begin() + CHUNKS, data.begin());
    }
    bool all() const {
        for (auto& x: data)
            if (x != ~0LLU)
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
        for (size_t i = 0; i < CHUNKS; i++) {
            if (data[i] != 0)
                return i * BITS_PER_CHUNK + __builtin_ctzll(data[i]);
        }
        return std::numeric_limits<T>::max();
    };
    size_t count_leading_zeros() const {
        for (size_t i = CHUNKS; i--;) {
            if (data[i] != 0)
                return i * BITS_PER_CHUNK + __builtin_clzll(data[i]);
        }
        return std::numeric_limits<T>::max();
    };
    friend bool operator==(const self_type& lhs, const self_type& rhs) {
        for (size_t i = 0; i < CHUNKS; i++)
            if (lhs.data[i] != rhs.data[i])
                return false;
        return true;
    };
    friend bool operator!=(const self_type& lhs, const self_type& rhs) {
        return !(lhs == rhs);
    };
    constexpr bool operator[](size_t pos) const {
        return (data[pos / BITS_PER_CHUNK] >> (pos % BITS_PER_CHUNK)) & 1;
    };
    void set() {
        for (auto& x: data)
            x = ~0LLU;
    };
    constexpr void set(size_t pos, bool value = true) {
        if (value) {
            data[pos / BITS_PER_CHUNK] |= 1LLU << (pos % BITS_PER_CHUNK);
        } else {
            data[pos / BITS_PER_CHUNK] &= ~(1LLU << (pos % BITS_PER_CHUNK));
        }
    };
    void reset() {
        for (auto& x: data)
            x = 0LLU;
    };
    void reset(size_t pos) {
        data[pos / BITS_PER_CHUNK] &= ~(1LLU << (pos % BITS_PER_CHUNK));
    };
    void flip() {
        for (auto& x: data)
            x ^= ~0LLU;
    };
    void flip(size_t pos) {
        data[pos / BITS_PER_CHUNK] ^= 1LLU << (pos % BITS_PER_CHUNK);
    };
    friend void operator~(self_type& lhs) {
        for (auto& x: lhs.data)
            x = ~x;
    };
    friend void operator&=(self_type& lhs, const self_type& rhs) {
        for (size_t i = 0; i < CHUNKS; i++)
            lhs.data[i] &= rhs.data[i];
    };
    friend void operator|=(self_type& lhs, const self_type& rhs) {
        for (size_t i = 0; i < CHUNKS; i++)
            lhs.data[i] |= rhs.data[i];
    };
    friend void operator^=(self_type& lhs, const self_type& rhs) {
        for (size_t i = 0; i < CHUNKS; i++)
            lhs.data[i] ^= rhs.data[i];
    };
    friend void operator&(self_type& lhs, const self_type& rhs) {
        self_type x = lhs;
        x &= rhs;
        return x;
    };
    friend void operator|(self_type& lhs, const self_type& rhs) {
        self_type x = lhs;
        x |= rhs;
        return x;
    };
    friend void operator^(self_type& lhs, const self_type& rhs) {
        self_type x = lhs;
        x ^= rhs;
        return x;
    };
    void operator>>=(size_t shift) {
        for (size_t i = 0; i + shift / BITS_PER_CHUNK < CHUNKS; i++) {
            data[i] = data[i + shift / BITS_PER_CHUNK] >> (shift % BITS_PER_CHUNK);
            if (i + 1 + shift / BITS_PER_CHUNK < CHUNKS)
                data[i] |= data[i + 1 + shift / BITS_PER_CHUNK] << (BITS_PER_CHUNK - shift % BITS_PER_CHUNK);
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
    template <size_t Step, size_t Offset>
    friend bitarray<N / Step, T> gather(const self_type& in) {
        //the output bit n comes from the input bit n*Step+Offset
        static_assert(N % Step == 0, "gather: the bitset size should be an exact multiple of the spread size");
        bitarray<N / Step, T> out{};
        //should loop over the input space instead of the output
        for (size_t i = 0; i < CHUNKS; i++) {
            //TODO cant have a runtime template parameter
            T tmp = _pext_u64(in.data[i], mask<Step, (i * BITS_PER_CHUNK) % Step>());
            out.data[i / Step] |= tmp << (i * BITS_PER_CHUNK / Step);
        }
        return out;
    };
    template <class CharT, class Traits>
    friend std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const self_type& x) {
        //FIXME decide what order the chunks should be stored in memory
        //currently the memory layout doesn't match this print
        for (size_t i = CHUNKS; i--;) {
            const T& chunk = x.data[i];
            for (size_t j = BITS_PER_CHUNK; j--;) {
                bool bit = (chunk >> j) & 1;
                os << (bit ? '1' : '0');
            }
        }
        return os;
    };
};
