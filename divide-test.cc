#include "bitarray.hh"
#include <cstdint>
#include <cassert>

int main() {
    for (ssize_t y: {-4, 4}) {
        for (ssize_t x = -16; x < 16; x++) {
            auto [q, r] = euclid_divide<ssize_t>(x, y);
            assert(q * y + r == x && r >= 0 && r < std::abs(y));
        }
    }
}
