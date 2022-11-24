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

template <std::size_t... Is>
struct seq {
};

template <std::size_t N, std::size_t... Is>
struct gen_seq : gen_seq<N - 1, N - 1, Is...> {
};

template <std::size_t... Is>
struct gen_seq<0, Is...> : seq<Is...> {
};

template <class Generator, std::size_t... Is>
constexpr auto generate_array_helper(Generator g, seq<Is...>)
        -> std::array<decltype(g(std::size_t{}, sizeof...(Is))), sizeof...(Is)> {
    return {{g(Is, sizeof...(Is))...}};
}

template <std::size_t N, class Generator>
constexpr auto generate_array(Generator g)
        -> decltype(generate_array_helper(g, gen_seq<N>{})) {
    return generate_array_helper(g, gen_seq<N>{});
}

}// namespace entropy