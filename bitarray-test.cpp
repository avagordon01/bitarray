#include "bitarray.hpp"
#include <cassert>
#include <iostream>

int main() {
    bitarray<129> x;
    x.data = {-1LLU, -1LLU, -1LLU};
    assert(x.all());
    x.data = {0, 0, 0};
    assert(x.none());
    x.data = {0, 0, 0};
    assert(!x.any());
    x.data = {1, 0, 0};
    assert(x.any());
    x.data = {1, 2, 3};
    assert(x.count() == 4);

    x.data = {-1LLU, -1LLU, 0};
    std::cout << x << std::endl;
    x >>= 8;
    std::cout << x << std::endl;

    x.data = {1, 2, 3};
    assert(x.count() == 4);

    return 0;
}
