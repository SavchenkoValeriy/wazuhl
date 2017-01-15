#include "llvm/Wazuhl/Random.h"
#include <cstddef>
#include <random>

namespace {
  const auto seed = std::random_device{}();
  std::mt19937 generator{seed};
}

namespace llvm {
namespace wazuhl {
namespace random {
  template <typename T>
  T getRandomNumberFromRange(T from, T to) {
    static std::uniform_int_distribution<T> distribution{from, to};
    return distribution(generator);
  }

  template <typename T>
  T getRandomNumberFromRange(T to) {
    return getRandomNumberFromRange<T>(0, to);
  }

  bool flipACoin(double probability) {
    std::bernoulli_distribution distribution{probability};
    return distribution(generator);
  }

  template int getRandomNumberFromRange(int to);
  template std::size_t getRandomNumberFromRange(std::size_t to);
}
}
}
