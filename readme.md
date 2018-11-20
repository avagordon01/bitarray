# bitarray

This repo provides an alternative to C++'s [std::bitset](https://en.cppreference.com/w/cpp/utility/bitset).

As with std::bitset, bitarray supports arbitrary length arrays of bits.

It aims to be simpler, faster, and more featureful.
It can be a drop-in replacement for bitset, because it has a strict superset of features.

Faster, because by default it uses the largest word size available (although most platforms define long (which std::bitset uses) to also be the largest word size available), potentially doing half as many instructions for long bitsets. The word type can also be changed if you need to reduce the wasted bits of non-multiple-of-64(/32)bit sized bitarrays.

The added features are (or will be):
 - Directly access the underlying words
   - bitset only allows you to get the lowest unsigned long long's worth of bits
   - or output to a string of '1's and '0's...
 - Easily change the underlying word type for whatever performance/storage needs
 - Extended with useful bitwise operations
   - count\_{leading,trailing}\_zeros, supported by [clz and ctz](https://en.wikipedia.org/wiki/Find_first_set)
   - deposit/extract and interleave/deinterleave, supported by [pdep/pext](https://en.wikipedia.org/wiki/Bit\_Manipulation\_Instruction\_Sets#BMI2)
 - Based on an efficient map function so you can easily extend the bitarray with your own operations

bitarray has been written so that it should perform the minimal number of word operations given the input and output size.

Initially, you might expect a bitarray with N words to have to perform N shift instructions. This is not the case. A shift operation has to do 2N left shift or right shift operations per word, because (at least on x86) there is no way to shift over a word boundary with one instruction, you have to shift `x << offset` one way, then shift `x >> (wordsize - offset)` the otherway to calculate the overflow.

## Dependencies

 - A C++17 compiler, we use clang++ by default for better sanitisation

For testing and installing:
 - [Meson](https://mesonbuild.com/) `sudo apt install meson`

## Usage

Include `bitarray.hpp` and use `bitarray<N>` instead of `std::bitset<N>`

## Building and Testing

`test.sh`
