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
    ASSERT_TRUE((T{-1LLU, -1LLU, -1LLU}).all());
    ASSERT_TRUE((T{0LLU, 0LLU, 0LLU}).none());
    ASSERT_TRUE(!(T{0LLU, 0LLU, 0LLU}).any());
    ASSERT_TRUE((T{1LLU, 0LLU, 0LLU}).any());
    ASSERT_EQ((T{3LLU, 2LLU, 1LLU}).count(), 4);
}

size_t seed = 0xfeed;
size_t num_trials = 1'000;
std::mt19937 engine(seed);
std::uniform_int_distribution<uint64_t> input_distribution;

TEST(bitarray, fuzz_count){
    constexpr size_t len = 128 + 7;
    std::uniform_int_distribution<size_t> set_distribution(0, len - 1);
    for(size_t i = 0; i < num_trials; i++) {
        bitarray::bitarray<len> x {};
        size_t pos = set_distribution(engine);
        x.set(pos);
        ASSERT_EQ(x.count_leading_zeros(), len - 1 - pos);
        ASSERT_EQ(x.count_trailing_zeros(), pos);
    }
}

TEST(bitarray, fuzz_shift){
    std::uniform_int_distribution<size_t> shift_distribution(0, 128);
    for(size_t i = 0; i < num_trials; i++) {
        auto input = bitarray::bitarray<128>{input_distribution(engine), input_distribution(engine)};
        auto shift = shift_distribution(engine);
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

TEST(bitarray, fuzz_ctors){
    for(size_t i = 0; i < num_trials; i++) {
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
}

TEST(bitarray, fuzz_gather){
    for(size_t i = 0; i < num_trials; i++) {
        auto input = bitarray::bitarray<128>{input_distribution(engine), input_distribution(engine)};
        auto mask = bitarray::bitarray<120>{input_distribution(engine), input_distribution(engine)};
        auto output = input.template gather<120>(mask);
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
    for(size_t i = 0; i < num_trials; i++) {
        auto input = bitarray::bitarray<128>{input_distribution(engine), input_distribution(engine)};
        auto mask = bitarray::bitarray<140>{input_distribution(engine), input_distribution(engine)};
        auto output = input.template scatter<140>(mask);
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
    for(size_t i = 0; i < num_trials; i++) {
        auto inputs = std::array<bitarray::bitarray<128>, 3>{
            bitarray::bitarray<128>{input_distribution(engine), input_distribution(engine)},
            bitarray::bitarray<128>{input_distribution(engine), input_distribution(engine)},
            bitarray::bitarray<128>{input_distribution(engine), input_distribution(engine)},
        };
        auto output = bitarray::bitarray<128>::interleave(inputs);
        decltype(output) expected {};
        for (size_t x = 0; x < output.size(); x++) {
            expected.set(x, inputs[x % 3][x / 3]);
        }
        ASSERT_EQ(output, expected);
        auto outputs = bitarray::bitarray<128>::deinterleave<128, 3>(output);
        ASSERT_EQ(outputs, inputs);
    }
}
