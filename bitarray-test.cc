#include "bitarray.hh"
#include <iostream>
#include <random>
#include <gtest/gtest.h>

#ifdef TYPE
using type = TYPE;
#else
using type = uint64_t;
#endif

TEST(bitarray, basic_functions){
    using T = bitarray::bitarray<129, type>;
    ASSERT_TRUE((T{~0LLU, ~0LLU, ~0LLU}).all());
    ASSERT_TRUE((T{0LLU, 0LLU, 0LLU}).none());
    ASSERT_FALSE((T{0LLU, 0LLU, 0LLU}).any());
    ASSERT_TRUE((T{1LLU, 0LLU, 0LLU}).any());
    ASSERT_EQ((T{3LLU, 2LLU, 1LLU}).count(), 4);
}

TEST(bitarray, fuzz_count){
    constexpr size_t len = 128 + 7;
    for(size_t i = 0; i < len; i++) {
        bitarray::bitarray<len> x {};
        size_t pos = i;
        x.set(pos);
        ASSERT_EQ(x.countl_zero(), len - 1 - pos);
        ASSERT_EQ(x.countr_zero(), pos);
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
