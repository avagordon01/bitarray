#include "bitarray.hh"
#include <iostream>
#include <gtest/gtest.h>

#include <vector>
#include <array>
#include <span>
#include <type_traits>

#ifdef TYPE
using type = TYPE;
#else
using type = uint64_t;
#endif

TEST(bitarray, bit_word_calculations) {
    ASSERT_EQ(bitarray::detail::words_needed<uint8_t>(0), 0);
    ASSERT_EQ(bitarray::detail::words_needed<uint8_t>(1), 1);
    ASSERT_EQ(bitarray::detail::words_needed<uint8_t>(8), 1);
    ASSERT_EQ(bitarray::detail::words_needed<uint8_t>(9), 2);
    ASSERT_EQ(bitarray::detail::words_needed<uint8_t>(16), 2);
    ASSERT_EQ(bitarray::detail::words_needed<uint8_t>(17), 3);
}

TEST(bitarray, ctors) {
    {
        bitarray::bitvector b(2);
        ASSERT_EQ(b.size(), 2);
        ASSERT_EQ(b.data().size(), 1);
        b.set(1, 1);
    }

    {
        bitarray::bitarray<100> b;
        b.set(1, 1);
    }

    {
        /*
        TODO
        std::vector<type> vector(100);
        bitarray::bitspan b(std::span<type, 100>{vector});
        b.set(1, 1);
        ASSERT_EQ(b.size(), 100);
        */
    }

    {
        std::vector<type> vector(100);
        bitarray::bitvector b(vector);
        b.set(1, 1);
        b.resize(3000);
        b.set(2000, 1);
    }

    {
        bitarray::bitvector b(2, {100, 100});
        b.set(1, 1);
        b.resize(3000);
        b.set(2000, 1);
    }

    {
        bitarray::bitvector b(2, std::vector<type>(100));
        b.set(1, 1);
        b.resize(3000);
        b.set(2000, 1);
    }

    {
        bitarray::bitarray<128> b;
        ASSERT_EQ(b.size(), 128);
    }

    {
        bitarray::bitvector b{128};
        ASSERT_EQ(b.size(), 128);
    }

    {
        bitarray::bitvector b(100);
        ASSERT_EQ(b.size(), 100);
        b.resize(200);
        ASSERT_EQ(b.size(), 200);
    }
}

TEST(bitarray, basic_functions){
    bitarray::bitarray<129>{{~0LLU, ~0LLU, ~0LLU}};
    ASSERT_TRUE((bitarray::bitarray<129>{{~0LLU, ~0LLU, ~0LLU}}).all());
    ASSERT_TRUE((bitarray::bitarray<129>{{0LLU, 0LLU, 0LLU}}).none());
    ASSERT_FALSE((bitarray::bitarray<129>{{0LLU, 0LLU, 0LLU}}).any());
    ASSERT_TRUE((bitarray::bitarray<129>{{1LLU, 0LLU, 0LLU}}).any());
    ASSERT_EQ((bitarray::bitarray<129>{{3LLU, 2LLU, 1LLU}}).count(), 4);
}

TEST(bitarray, fuzz_count){
    {
        constexpr size_t len = 128 + 7;
        for(size_t i = 0; i < len; i++) {
            bitarray::bitarray<len> x {};
            size_t pos = i;
            x.set(pos);
            ASSERT_EQ(x.countl_zero(), len - 1 - pos);
            ASSERT_EQ(x.countr_zero(), pos);
            x.flip();
            ASSERT_EQ(x.countl_one(), len - 1 - pos);
            ASSERT_EQ(x.countr_one(), pos);
        }
    }
    {
        constexpr size_t len = 128;
        for(size_t i = 0; i < len; i++) {
            bitarray::bitarray<len> x {};
            size_t pos = i;
            x.set(pos);
            ASSERT_EQ(x.countl_zero(), len - 1 - pos);
            ASSERT_EQ(x.countr_zero(), pos);
            x.flip();
            ASSERT_EQ(x.countl_one(), len - 1 - pos);
            ASSERT_EQ(x.countr_one(), pos);
        }
    }
}

TEST(bitarray, fuzz_shift){
    constexpr size_t len = 128;
    for(size_t i = 0; i < len; i++) {
        auto input = bitarray::bitarray<len>{};
        for (size_t j = 1; j < len; j *= 2) {
            input.set(j);
        }
        auto shift = i;
        auto output = input >> shift;
        decltype(output) expected {};
        for (size_t x = shift, y = 0; x < input.size() && y < output.size(); x++, y++) {
            expected.set(y, input[x]);
        }
        ASSERT_EQ(output, expected);
        output = input << shift;
        expected = {};
        for (size_t x = 0, y = shift; x < input.size() && y < output.size(); x++, y++) {
            expected.set(y, input[x]);
        }
        ASSERT_EQ(output, expected);
    }
}

#if 0
TEST(bitarray, fuzz_rotate){
    constexpr int len = 128;
    for(int i = -len; i < len; i++) {
        auto input = bitarray::bitarray<len>{};
        for (size_t j = 1; j < len; j *= 2) {
            input.set(j);
        }
        auto shift = i;
        auto output = input.rotl(shift);
        decltype(output) expected {};
        for (size_t x = 0; x < input.size(); x++) {
            expected.set((x + shift) % input.size(), input[x]);
        }
        ASSERT_EQ(output, expected);
    }
}

TEST(bitarray, fuzz_ctors){
    constexpr size_t l = 120;
    bitarray::bitarray<l> a {~0LLU, ~0LLU};
    bitarray::bitarray<l> b {};
    b.set();
    bitarray::bitarray<l> c {};
    for (size_t i = 0; i < c.size(); i++) {
        c.set(i, 1);
    }
    ASSERT_EQ(a, b);
    ASSERT_EQ(b, c);
}

TEST(bitarray, fuzz_gather){
    constexpr size_t len = 128;
    for(size_t i = 0; i < len; i++) {
        auto input = bitarray::bitarray<len>{};
        input.set(i);
        auto mask = bitarray::bitarray<len>{};
        for (size_t j = 1; j < len; j *= 2) {
            mask.set(j);
        }
        auto output = input.template gather<len>(mask);
        decltype(output) expected {};
        for (size_t x = 0, y = 0; x < mask.size() && x < input.size() && y < output.size(); x++) {
            if (mask[x]) {
                expected.set(y, input[x]);
                y++;
            }
        }
        ASSERT_EQ(output, expected);
    }
}

TEST(bitarray, fuzz_scatter){
    constexpr size_t len = 128;
    for(size_t i = 0; i < len; i++) {
        auto input = bitarray::bitarray<len>{};
        input.set(i);
        auto mask = bitarray::bitarray<len>{};
        for (size_t j = 1; j < len; j *= 2) {
            mask.set(j);
        }
        auto output = input.template scatter<len>(mask);
        decltype(output) expected {};
        for (size_t x = 0, y = 0; x < mask.size() && x < output.size() && y < input.size(); x++) {
            if (mask[x]) {
                expected.set(x, input[y]);
                y++;
            }
        }
        ASSERT_EQ(output, expected);
    }
}

TEST(bitarray, fuzz_interleave_deinterleave){
    constexpr size_t len = 128;
    auto inputs = std::array<bitarray::bitarray<len>, 3>{
        bitarray::bitarray<len>{~0ULL, ~0ULL},
        bitarray::bitarray<len>{0ULL, 0ULL},
        bitarray::bitarray<len>{},
    };
    for (size_t j = 1; j < len; j *= 2) {
        inputs[2].set(j);
    }
    auto output = bitarray::bitarray<len>::interleave(inputs);
    decltype(output) expected {};
    for (size_t x = 0; x < output.size(); x++) {
        expected.set(x, inputs[x % 3][x / 3]);
    }
    ASSERT_EQ(output, expected);
    auto outputs = bitarray::bitarray<len>::deinterleave<len, 3>(output);
    ASSERT_EQ(outputs, inputs);
}
#endif
