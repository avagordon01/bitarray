#include "bitarray.hpp"
#include <cassert>
#include <iostream>

int main() {
    using T = bitarray<129>;
    assert((T{-1LLU, -1LLU, -1LLU}).all());
    assert((T{0, 0, 0}).none());
    assert(!(T{0, 0, 0}).any());
    assert((T{1, 0, 0}).any());
    assert((T{1, 2, 3}).count() == 4);

    T x {-1LLU, -1LLU, 0};
    std::cout << x << std::endl;
    x >>= 8;
    std::cout << x << std::endl;

    assert((T{1, 2, 3}).count() == 4);

    assert((T{0, 1 << 10, 0}).count_trailing_zeros() == 64 + 10);
    assert((T{0, 1 << 10, 0}).count_leading_zeros() == 64 + 63 - 10);

    return 0;
}
