#ifndef LLVM_WAZUHL_RANDOM_H
#define LLVM_WAZUHL_RANDOM_H

#include <cstddef>

namespace llvm {
namespace wazuhl {
namespace random {
  template <typename T>
  T getRandomNumberFromRange(T from, T to);

  template <typename T>
  T getRandomNumberFromRange(T to);

  bool flipACoin(double probability);

  template <class Range>
  auto pickOutOf(const Range &values) -> decltype(*values.begin()) {
    std::size_t size = values.end() - values.begin();
    std::size_t randomIndex = getRandomNumberFromRange(size - 1);
    return *(values.begin() + randomIndex);
  }
}
}
}
#endif /* LLVM_WAZUHL_RANDOM_H */
