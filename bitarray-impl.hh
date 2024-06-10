template <typename T>
concept VariableSize = requires(T t) {
    t._size;
};

template <typename Specific, typename Traits>
struct bitarray_impl
{
    using Container = typename Traits::Container;
    using WordType = Container::value_type;
    using self_type = bitarray_impl<Specific, Traits>;
    static constexpr auto variable_sized = VariableSize<Specific>;
    Container data_;

    bitarray_impl() = default;
    bitarray_impl(Container c) : data_(c) {}
    template <typename Other>
    bitarray_impl(Other l)
    {
        std::copy(l.begin(), l.begin() + std::min(l.size(), data_.size()), data_.begin());
        sanitize();
    }

    static constexpr auto WordBits = std::numeric_limits<WordType>::digits;

    template <class CharT, class CTraits>
    friend std::basic_ostream<CharT, CTraits> &operator<<(std::basic_ostream<CharT, CTraits> &os, const self_type &x)
    {
        int base_digits = 0;
        if (os.flags() & std::ios_base::dec) {
            base_digits = 1; //XXX this is a hack, rather than default to decimal, we default to binary
            os << "0b";
        } else if (os.flags() & std::ios_base::oct) {
            base_digits = 3;
            os << "0o";
        } else if (os.flags() & std::ios_base::hex) {
            base_digits = 4;
            os << "0x";
        }
        for (ssize_t i = x.size(); i -= base_digits;) {
            unsigned char digit = 0;
            for (int j = 0; j < base_digits; j++) {
                digit <<= 1;
                digit += x[i + j];
            }
            os << std::to_string(digit);
        }
        return os;
    }

    size_t size() const
    {
        if constexpr (!variable_sized)
        {
            return std::size(data_) * WordBits;
        }
        else
        {
            if (Specific::_size == std::dynamic_extent)
            {
                return std::size(data_) * WordBits;
            }
            else
            {
                return Specific::_size;
            }
        }
    }

    Container &data()
    {
        return data_;
    }

private:
    constexpr WordType zero() const {
        return static_cast<WordType>(0);
    }
    constexpr WordType one() const {
        return static_cast<WordType>(1);
    }
    constexpr WordType ones() const {
        return ~static_cast<WordType>(0);
    }

protected:
    void sanitize() {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshift-count-overflow"
        if (size() % WordBits != 0) {
            data_.back() &= ones() >> (WordBits - size() % WordBits);
        }
#pragma GCC diagnostic pop
    }

public:
    bool all() const {
        if (size() % WordBits == 0) {
            for (size_t i = 0; i < std::size(data_); i++)
                if (data_[i] != ones())
                    return false;
        } else {
            if (data_.back() != ones() >> (WordBits - size() % WordBits))
                return false;
            for (size_t i = 0; i + 1 < std::size(data_); i++)
                if (data_[i] != ones())
                    return false;
        }
        return true;
    }
    bool any() const {
        for (auto &x : data_)
            if (x != 0)
                return true;
        return false;
    }
    bool none() const {
        for (auto &x : data_)
            if (x != 0)
                return false;
        return true;
    }
    int popcount() const {
        return count();
    }
    size_t count() const {
        size_t count = 0;
        for (auto &x : data_)
            if constexpr (sizeof(WordType) <= 8) {
                count += std::popcount(x);
            } else if (sizeof(WordType) <= 16) {
                count += std::popcount(static_cast<uint64_t>(x >> 64)) + std::popcount(static_cast<uint64_t>(x));
            }
        return count;
    }
    bool has_single_bit() const {
        return count() == 1;
    }
    int countr_zero() const {
        for (size_t i = 0; i < std::size(data_); i++)
            if (data_[i] != 0)
                return i * WordBits + std::countr_zero(data_[i]);
        return size();
    }
    int countr_one() const {
        for (size_t i = 0; i < std::size(data_); i++)
            if (data_[i] != ones())
                return i * WordBits + std::countr_one(data_[i]);
        return size();
    }
    int countl_zero() const {
        for (size_t i = std::size(data_); i--;)
            if (data_[i] != 0)
                return size() - i * WordBits - (WordBits - std::countl_zero(data_[i]));
        return size();
    }
    int countl_one() const {
        size_t i = std::size(data_);
        if (size() % WordBits != 0) {
            if (data_.back() != (ones() >> (WordBits - size() % WordBits)))
                return std::countl_one(data_.back() << (WordBits - size() % WordBits));
            i--;
        }
        for (; i--;)
            if (data_[i] != ones())
                return size() - i * WordBits - (WordBits - std::countl_one(data_[i]));
        return size();
    }
    int bit_width() const {
        return size() - countl_zero();
    }
    self_type bit_floor() const {
        int w = bit_width();
        reset();
        if (w != 0) {
            set(w - 1);
        }
        return *this;
    }
    self_type bit_ceil() const {
        int w = bit_width();
        bool x = has_single_bit();
        reset();
        if (x) {
            set(w - 1);
        } else {
            set(w);
        }
        return *this;
    }





    void wordswap() {
        std::reverse(std::begin(data_), std::end(data_));
    }
    void byteswap() {
        wordswap();
        for (auto &x : data_)
        {
            //std::byteswap(x);
        }
    }
    void bitswap() {
        byteswap();
        //TODO bitswap
    }
    self_type set() {
        for (auto &x : data_)
            x = ones();
        sanitize();
        return *this;
    }
    constexpr self_type set(size_t pos, bool value = true) {
        if (pos >= size()) {
            throw std::out_of_range{"set() called with pos " + std::to_string(pos) + " on bitset of size " + std::to_string(size())};
        }
        if (value) {
            data_[pos / WordBits] |= one() << (pos % WordBits);
        } else {
            data_[pos / WordBits] &= ~(one() << (pos % WordBits));
        }
        return *this;
    }
    self_type reset() {
        for (auto &x : data_)
            x = zero();
        return *this;
    }
    self_type reset(size_t pos) {
        if (pos >= size()) {
            throw std::out_of_range{"reset() called with pos " + std::to_string(pos) + " on bitset of size " + std::to_string(size())};
        }
        data_[pos / WordBits] &= ~(one() << (pos % WordBits));
        return *this;
    }
    self_type flip() {
        for (auto &x : data_)
            x = ~x;
        sanitize();
        return *this;
    }
    self_type flip(size_t pos) {
        if (pos >= size()) {
            throw std::out_of_range{"flip() called with pos " + std::to_string(pos) + " on bitset of size " + std::to_string(size())};
        }
        data_[pos / WordBits] ^= one() << (pos % WordBits);
        return *this;
    }
    void set_word_at_pos(WordType x, size_t pos) {
        if (pos >= size()) {
            return;
        }
        size_t offset = pos % WordBits;
        data_[pos / WordBits] |= x << offset;
        if (offset != 0 && pos / WordBits + 1 < std::size(data_))
        {
            data_[pos / WordBits + 1] |= x >> (WordBits - offset);
        }
    }
    WordType get_word_at_pos(size_t pos) const {
        if (pos >= size()) {
            throw std::out_of_range{"get_word_at_pos() called with pos " + std::to_string(pos) + " on bitset of size " + std::to_string(size())};
        }
        size_t offset = pos % WordBits;
        WordType out = data_[pos / WordBits] >> offset;
        if (offset != 0 && pos / WordBits + 1 < std::size(data_))
        {
            out |= data_[pos / WordBits + 1] << (WordBits - offset);
        }
        return out;
    }
    bool operator==(const self_type& rhs) const {
        for (size_t i = 0; i < std::size(data_); i++)
            if (data_[i] != rhs.data_[i])
                return false;
        return true;
    }
    bool operator!=(const self_type& rhs) const {
        return !(*this == rhs);
    }
    constexpr bool at(size_t pos) const {
        if (pos >= size()) {
            throw std::out_of_range{"at() called with pos " + std::to_string(pos) + " on bitset of size " + std::to_string(size())};
        }
        return *this[pos];
    }
    constexpr bool operator[](size_t pos) const {
        return static_cast<bool>((data_[pos / WordBits] >> (pos % WordBits)) & 1);
    }

    self_type operator&=(const self_type& rhs) {
        for (size_t i = 0; i < std::size(data_); i++)
            data_[i] &= rhs.data_[i];
        return *this;
    }
    self_type operator|=(const self_type& rhs) {
        for (size_t i = 0; i < std::size(data_); i++)
            data_[i] |= rhs.data_[i];
        return *this;
    }
    self_type operator^=(const self_type& rhs) {
        for (size_t i = 0; i < std::size(data_); i++)
            data_[i] ^= rhs.data_[i];
        sanitize();
        return *this;
    }
    self_type operator<<=(size_t shift) {
        for (size_t i = std::size(data_); i--;)
        {
            auto x = data_[i];
            data_[i] = 0;
            set_word_at_pos(x, i * WordBits + shift);
        }
        return *this;
    }
    self_type operator<<(size_t shift) {
        self_type x = *this;
        x <<= shift;
        return x;
    }
    self_type operator>>=(size_t shift) {
        for (size_t i = 0; i < std::size(data_); i++)
        {
            if (shift + i * WordBits < size()) {
                data_[i] = get_word_at_pos(shift + i * WordBits);
            } else {
                data_[i] = 0;
            }
        }
        return *this;
    }
    self_type operator>>(size_t shift) {
        self_type x = *this;
        x >>= shift;
        return x;
    }
    self_type rotl(int shift) {
        if (shift < 0) {
            return rotr(-shift);
        }
        shift %= size();
        //FIXME implement without temporaries
        //return (*this << shift) | (*this >> (size() - shift));
        return *this;
    }
    self_type rotr(int shift) {
        if (shift < 0) {
            return rotl(-shift);
        }
        shift %= size();
        //FIXME implement without temporaries
        //return (*this >> shift) | (*this << (size() - shift));
        return *this;
    }

    /*
    FIXME
    template<size_t M, size_t O = M>
    bitarray<O, WordType> gather(bitarray<M, WordType> mask) {
        static_assert(M <= size(), "gather operation mask length must be <= input length");
        bitarray<O, WordType> output {};
        for (size_t i = 0, pos = 0; i < mask.std::size(data_); i++) {
            output.set_word_at_pos(pext(
                data_[i],
                mask.data_[i]
            ), pos);
            pos += std::popcount(mask.data_[i]);
        }
        return output;
    }
    template<size_t M>
    bitarray<M, WordType> scatter(bitarray<M, WordType> mask) {
        static_assert(M >= size(), "scatter operation mask length must be >= input length");
        bitarray<M, WordType> output {};
        for (size_t i = 0, pos = 0; i < output.std::size(data_); i++) {
            output.data_[i] = pdep(
                get_word_at_pos(pos),
                mask.data_[i]
            );
            pos += std::popcount(mask.data_[i]);
        }
        return output;
    }


    template<size_t Len, size_t Num>
    static constexpr std::array<bitarray<Len>, Num> interleave_masks() {
        std::array<bitarray<Len>, Num> x{};
        for (size_t i = 0; i < Num; i++) {
            for (size_t j = i; j < Len; j += Num) {
                x[i].set(j);
            }
        }
        return x;
    }

    template<size_t Len, size_t Num>
    static bitarray<Len * Num> interleave(std::array<bitarray<Len>, Num> input) {
        bitarray<Len * Num> output {};
        std::array<bitarray<Len * Num>, Num> masks = interleave_masks<Len * Num, Num>();
        for (size_t j = 0; j < input.size(); j++) {
            output |= input[j].template scatter<Len * Num>(masks[j]);
        }
        return output;
    }
    template<size_t Len, size_t Num>
    static std::array<bitarray<Len>, Num> deinterleave(bitarray<Len * Num> input) {
        std::array<bitarray<Len>, Num> output {};
        std::array<bitarray<Len * Num>, Num> masks = interleave_masks<Len * Num, Num>();
        for (size_t j = 0; j < output.size(); j++) {
            output[j] = input.template gather<Len * Num, Len>(masks[j]);
        }
        return output;
    }
    */
};
