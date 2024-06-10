#include <cstdint>
#include <limits>
#include <iostream>
#include <bit>
#include <stdexcept>
#include <algorithm>
#include <optional>
#include <array>
#include <vector>
#include <span>

#include "pdep-pext.hh"
#include "bitarray-impl.hh"

namespace bitarray::detail
{
    template<typename WordType>
    constexpr size_t words_needed(size_t bits) {
        if (bits == std::dynamic_extent)
        {
            return std::dynamic_extent;
        }
        else
        {
            size_t WordBits = std::numeric_limits<WordType>::digits;
            return (bits + WordBits - 1) / WordBits;
        }
    }
    template<typename Container>
    constexpr size_t bits_in_container(Container data) {
        return data.size() * std::numeric_limits<typename Container::value_type>::digits;
    }
}

namespace bitarray {
    template <size_t Bits, typename WordType>
    struct bitarray_traits
    {
        using Container = std::array<WordType, detail::words_needed<WordType>(Bits)>;
    };

    template <size_t Bits, typename WordType = size_t>
    struct bitarray : bitarray_impl<bitarray<Bits, WordType>, bitarray_traits<Bits, WordType>>
    {
        using self_type = bitarray<Bits, WordType>;
        using base_type = bitarray_impl<bitarray<Bits, WordType>, bitarray_traits<Bits, WordType>>;

        bitarray()
        {
            base_type::sanitize();
        }
        bitarray(std::initializer_list<WordType> l) : base_type(l) {}
    };

    template <typename WordType>
    struct bitvector_traits
    {
        using Container = std::vector<WordType>;
    };

    template <typename WordType = size_t>
    struct bitvector : bitarray_impl<bitvector<WordType>, bitvector_traits<WordType>>
    {
        using self_type = bitvector<WordType>;
        using base_type = bitarray_impl<bitvector<WordType>, bitvector_traits<WordType>>;

        size_t _size = std::dynamic_extent;

        bitvector() {}
        bitvector(std::vector<WordType> v) : base_type(v)
        {
            _size = detail::bits_in_container(base_type::data_);
        }
        bitvector(size_t size) : _size(size)
        {
            resize(_size);
        }
        bitvector(size_t size, std::vector<WordType> v) : base_type(v), _size(size)
        {
            base_type::sanitize();
        }
        void resize(size_t s)
        {
            size_t needed = detail::words_needed<WordType>(s);
            if (std::size(base_type::data_) < needed)
            {
                base_type::data_.resize(needed);
            }
            _size = s;
        }
    };

    template <size_t Bits, typename WordType>
    struct bitspan_traits
    {
        using Container = std::span<WordType, detail::words_needed<WordType>(Bits)>;
    };

    template <size_t Bits = std::dynamic_extent, typename WordType = size_t>
    struct bitspan : bitarray_impl<bitspan<Bits, WordType>, bitspan_traits<Bits, WordType>>
    {
        using self_type = bitspan<Bits, WordType>;
        using base_type = bitarray_impl<bitspan<Bits, WordType>, bitspan_traits<Bits, WordType>>;

        size_t _size;

        bitspan(std::span<WordType, Bits> s) : base_type(s), _size(Bits) {}
        bitspan(size_t size, std::span<WordType, Bits> s) : base_type(s), _size(size)
        {
            base_type::sanitize();
        }
        void resize(size_t s)
        {
            size_t needed = detail::words_needed<WordType>(s);
            if (std::size(base_type::data_) < needed)
            {
                base_type::data_.resize(needed);
            }
            _size = s;
        }

        self_type operator<<(size_t) = delete;
        self_type operator>>(size_t) = delete;
    };
}
