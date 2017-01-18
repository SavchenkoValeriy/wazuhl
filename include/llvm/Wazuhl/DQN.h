#ifndef LLVM_WAZUHL_DQN_H
#define LLVM_WAZUHL_DQN_H

#include <memory>

namespace llvm {
namespace wazuhl {

  class DQNImpl;

  class DQN {
  public:
    // TODO: Change to true values
    using State = int;
    using Action = int;
    using Result = double;

    Result &operator() (const State &S, const Action &A);
    Result operator() (const State &S, const Action &A) const;

  private:
    std::unique_ptr<DQNImpl> pImpl;
  };
}
}

#endif /* LLVM_WAZUHL_DQN_H */
