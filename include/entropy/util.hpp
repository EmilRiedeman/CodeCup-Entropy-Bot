#pragma once

#include <array>
#include <random>

namespace entropy {

struct FastRand {
    typedef uint result_type;

    result_type seed = std::random_device()();

    constexpr result_type operator()() {
        seed = (214013 * seed + 2531011);
        return seed >> 17;
    }

    [[nodiscard]] constexpr static result_type min() { return 0; }

    [[nodiscard]] constexpr static result_type max() { return std::numeric_limits<int16_t>::max(); }
};

template <typename RandomAccessIterator, typename Generator>
RandomAccessIterator random_element(RandomAccessIterator begin, typename std::remove_reference_t<Generator>::result_type n, Generator &&gen) {
    return begin +
           std::uniform_int_distribution<typename std::remove_reference_t<Generator>::result_type>{
                   0, n - 1}(std::forward<Generator>(gen));
}

template <uint N>
constexpr uint int_pow(uint p) {
    if (p == 1) return N;
    if (p == 0) return 1;

    return int_pow<N>(p - 1) * N;
}

template <typename T>
struct LookupPow {
    template <T X, T P>
    struct pow {
        constexpr static auto result = X * pow<X, P - 1>::result;
    };

    template <T X, T P>
    constexpr static inline T calculate = pow<X, P>::result;

    template <T X>
    struct pow<X, 0> {
        constexpr static auto result = 1;
    };

    template <T X>
    struct pow<X, 1> {
        constexpr static auto result = X;
    };
};

template <typename Generator, std::size_t... Is>
constexpr auto generate_array(Generator &&g, std::index_sequence<Is...>)
        -> std::array<decltype(std::forward<Generator>(g)(std::size_t{}, sizeof...(Is))), sizeof...(Is)> {
    return {{std::forward<Generator>(g)(Is, sizeof...(Is))...}};
}

template <std::size_t N, typename Generator>
constexpr decltype(auto) generate_array(Generator &&g) {
    return generate_array_helper(std::forward<Generator>(g), std::make_index_sequence<N>{});
}

template <typename T, std::size_t... Is>
constexpr std::array<T, sizeof...(Is)> create_array(T value, std::index_sequence<Is...>) {
    return {{(static_cast<void>(Is), value)...}};
}

template <std::size_t N, typename T>
constexpr std::array<T, N> create_array(const T &value) {
    return create_array(value, std::make_index_sequence<N>());
}

}// namespace entropy