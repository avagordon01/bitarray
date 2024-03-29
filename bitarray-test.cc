#include "bitarray.hh"
#include <iostream>
#include <gtest/gtest.h>
#include <vector>
#include <span>

#ifdef TYPE
using type = TYPE;
#else
using type = uint64_t;
#endif

TEST(bitarray, ctors){
    using T = bitarray::bitarray<257, type>;
    T{};
    T{0};
    T{0, 0};
    T{0, 0, 0};
}

TEST(bitarray, vector){
    using T = bitarray::bitarray<257, type, std::vector<type>>;
    T{};
    T{0};
    T{0, 0};
    T{0, 0, 0};

    bitarray::bitarray<bitarray::match_underlying, type, std::vector<type>> b;
    b = {};
    ASSERT_EQ(b.size(), sizeof(type) * 8 * 0);
    b = {0};
    ASSERT_EQ(b.size(), sizeof(type) * 8 * 1);
    b = {0, 0};
    ASSERT_EQ(b.size(), sizeof(type) * 8 * 2);
    b = {0, 0, 0};
    ASSERT_EQ(b.size(), sizeof(type) * 8 * 3);

    bitarray::bitarray<0, type, std::vector<type>> x;
    x.resize(127);
    ASSERT_EQ(b.size(), 127);
    x.resize(127 * 1024);
    ASSERT_EQ(b.size(), 127 * 1024);
}

TEST(bitarray, span){
    using T = bitarray::bitarray<257, type, std::span<type>>;
    std::vector<type> v {128};
    std::span{v};
    T{std::span{v}};
}

TEST(bitarray, deduction){
    {
        bitarray::bitarray b{std::array<type, 2>{}};
        ASSERT_EQ(b.size(), sizeof(type) * 8 * 2);
    }
    {
        bitarray::bitarray b{std::vector<type>{3}};
        ASSERT_EQ(b.size(), sizeof(type) * 8);
    }
    {
        std::vector<type> v(30);
        std::span<type> s{v.data(), v.size()};
        bitarray::bitarray b{s};
        ASSERT_EQ(b.size(), sizeof(type) * 8 * 30);
    }
    {
        std::vector<type> v(30);
        std::span<type> s{v.data(), 2};
        bitarray::bitarray b{s};
        ASSERT_EQ(b.size(), sizeof(type) * 8 * 2);
    }
}

TEST(bitarray, basic_functions){
    using T = bitarray::bitarray<257, type>;
    ASSERT_TRUE((~T{}).all());
    ASSERT_TRUE((T{0, 0, 0}).none());
    ASSERT_FALSE((T{0, 0, 0}).any());
    ASSERT_TRUE((T{1, 0, 0}).any());
    ASSERT_EQ((T{3, 2, 1}).count(), 4);
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
            x = ~x;
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
            x = ~x;
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
    bitarray::bitarray<l> a {};
    a.set();
    bitarray::bitarray<l> b {};
    for (size_t i = 0; i < b.size(); i++) {
        b.set(i, 1);
    }
    ASSERT_EQ(a, b);
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
        ~bitarray::bitarray<len>{},
        bitarray::bitarray<len>{},
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
