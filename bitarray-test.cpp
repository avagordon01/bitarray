#include "bitarray.hpp"
#include <cassert>
#include <iostream>

uint64_t f(uint64_t x) {
    return x;
};
struct g {
    size_t operator()(size_t offset) {
        return offset + 1;
    };
};

int main() {
    using T = bitarray<129>;
    assert((T{-1LLU, -1LLU, -1LLU}).all());
    assert((T{0, 0, 0}).none());
    assert(!(T{0, 0, 0}).any());
    assert((T{1, 0, 0}).any());
    assert((T{1, 2, 3}).count() == 4);

    assert((T{1, 2, 3}).count() == 4);

    assert((T{0, 1 << 10, 0}).count_trailing_zeros() == 64 + 10);
    assert((T{0, 1 << 10, 0}).count_leading_zeros() == 64 + 63 - 10);

    bitarray<192> input{~0LLU, 0LLU, 0LLU}, output{};
    std::cout << input << std::endl;
    input = input >> 1;
    std::cout << input << std::endl;

    return 0;
}
