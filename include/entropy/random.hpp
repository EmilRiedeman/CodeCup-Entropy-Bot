#pragma once

#include <array>
#include <random>

namespace entropy {

template <typename InputIterator, typename Generator>
InputIterator random_element(InputIterator begin, typename std::remove_reference_t<Generator>::result_type n, Generator &&gen) {
    std::advance(begin, std::uniform_int_distribution<typename std::remove_reference_t<Generator>::result_type>{
                                0, n - 1}(std::forward<Generator>(gen)));
    return begin;
}

struct FastRand {
    typedef uint32_t result_type;

    result_type seed = std::random_device()();

    constexpr result_type operator()() {
        seed = (214013 * seed + 2531011);
        return seed >> 17;
    }

    constexpr static result_type min() { return 0; }

    constexpr static result_type max() { return std::numeric_limits<int16_t>::max(); }
};

class MersenneTwisterEngine64 {
public:
    typedef std::uint64_t result_type;

    constexpr explicit MersenneTwisterEngine64(result_type seed) : state{seed} {
        for (std::size_t i = 1; i < N; ++i)
            state[i] = F * (state[i - 1] ^ (state[i - 1] >> 62)) + i;
    }

    constexpr result_type operator()() {
        if (index == N) twist();
        result_type y = state[index];
        y ^= (y >> U) & D;
        y ^= (y << S) & B;
        y ^= (y << T) & C;
        y ^= y >> L;
        ++index;
        return y;
    }

    constexpr static result_type min() { return 0; }

    constexpr static result_type max() { return std::numeric_limits<result_type>::max(); }

private:
    static constexpr std::size_t N = 312;
    static constexpr result_type A = 0xb5026f5aa96619e9;
    static constexpr result_type U = 29;
    static constexpr result_type D = 0x5555555555555555;
    static constexpr result_type S = 17;
    static constexpr result_type B = 0x71d67fffeda60000;
    static constexpr result_type T = 37;
    static constexpr result_type C = 0xfff7eee000000000;
    static constexpr result_type L = 43;
    static constexpr result_type F = 6364136223846793005;

    constexpr void twist() {
        for (std::size_t i = 0; i < N; ++i) {
            result_type x = (state[i] & 0xffffffffffffffffull) + (state[(i + 1) % N] & 0x7fffffffffffffffull);
            state[i] = state[(i + N / 2) % N] ^ (x >> 1);
            if (x % 2 != 0) {
                state[i] ^= A;
            }
        }
        index = 0;
    }

    std::array<result_type, N> state;
    std::size_t index = N;
};

}// namespace entropy