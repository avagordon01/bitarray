# bitarray / bitvector / bitspan

This repo provides an alternative to C++'s [`std::bitset`](https://en.cppreference.com/w/cpp/utility/bitset) with all the features of C++20's [`<bit>` header](https://en.cppreference.com/w/cpp/header/bit).

As with `std::bitset`, `bitarray` supports arbitrary length arrays of bits. It also supports dynamic bit lengths `bitvector` (on `std::vector`). It supports mmap'd bitsets `bitspan` (via `std::span`).

It aims to be simpler, faster, and more featureful.

Faster, because:
- most operations are in-place, to eliminate large copies
- the word size can be larger to reduce the number of iterations. potentially doing half as many iterations, std::bitset uses long
- or the word size can be smaller to reduce wasted space (relevant for many small bitsets)

The added features are (or will be):
- [ ] Easy casts between `bitarray`, `bitvector`, `bitspan`
- [ ] Easy constructors from `std::array`, `std::vector`, `std::span`, `std::initializer_list`
- [x] Directly access all the underlying words
  - bitset only allows you to get the lowest unsigned long long's worth of bits
  - or output to a string of '1's and '0's...
- [x] Easily change the underlying word type for whatever performance/storage needs (8/16/32/64/128 bit types are all supported and tested)
- [x] Supports all [C++20 <bit>](https://en.cppreference.com/w/cpp/header/bit) bitwise operations
  - popcount, rotl, rotr, count\_{l,r}\_{zero,one}
- [x] deposit/extract and interleave/deinterleave, supported by [pdep/pext](https://en.wikipedia.org/wiki/Bit\_Manipulation\_Instruction\_Sets#BMI2)

## Dependencies

- C++20
- Target CPU with support for the [BMI2 instruction set](https://en.wikipedia.org/wiki/Bit\_Manipulation\_Instruction\_Sets#BMI2) (for pdep/pext support, which gather/scatter and interleave/deinterleave rely on)

For testing and installing:
- [Meson](https://mesonbuild.com/) `sudo apt install meson`

## Usage

Include `bitarray.hh` and use `bitarray::bitarray<N>` instead of `std::bitset<N>`, or `bitarray::bitvector(N)`, or `bitarray::bitspan(span)`

## Testing

`test.sh`
