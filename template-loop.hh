#include <type_traits>

template <int I>
struct For {
    template <typename F>
    For(F&& f) {
        For<I-1>(std::forward<F>(f));
        f(I-1);
    }
};

template<>
struct For<0> {
    template <typename F>
    For(F) {}
};
