#ifndef LLVM_WAZUHL_DQN_H
#define LLVM_WAZUHL_DQN_H

#include "llvm/Wazuhl/PassAction.h"
#include "llvm/Wazuhl/Q.h"
#include "llvm/Wazuhl/StateFeatures.h"
#include <memory>
#include <vector>

namespace llvm {
namespace wazuhl {

class DQNCoreImpl;

class DQNCore {
public:
  using State = StateFeatures;
  using Action = PassAction;
  using Result = double;
  using ResultsVector = std::vector<Result>;

  ResultsVector calculate(const State &S) const;
  void update(const State &S, const Action &A, Result value);

  DQNCore();
  ~DQNCore();

private:
  std::unique_ptr<DQNCoreImpl> pImpl;
};

using DQN = rl::Q<DQNCore>;
} // namespace wazuhl
} // namespace llvm

#endif /* LLVM_WAZUHL_DQN_H */
