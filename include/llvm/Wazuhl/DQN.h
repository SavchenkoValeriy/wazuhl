#ifndef LLVM_WAZUHL_DQN_H
#define LLVM_WAZUHL_DQN_H

#include "llvm/Wazuhl/Action.h"
#include "llvm/Wazuhl/FeatureCollector.h"
#include "llvm/Wazuhl/Q.h"
#include <memory>
#include <vector>

namespace llvm {
namespace wazuhl {

  class DQNCore {
  public:
    using State = FeatureVector;
    using Action = Action;
    using Result = double;
    using ResultsVector = std::vector<Result>;

    ResultsVector calculate(const State &S) const;
    void update(const State &S, const Action &A, Result value);
  };

  using DQN = rl::Q<DQNCore>;
}
}

#endif /* LLVM_WAZUHL_DQN_H */
