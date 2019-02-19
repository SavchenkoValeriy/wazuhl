#ifndef LLVM_WAZUHL_ENVIRONMENT_H
#define LLVM_WAZUHL_ENVIRONMENT_H

#include "llvm/Wazuhl/FeatureCollector.h"
#include "llvm/Wazuhl/PassAction.h"

namespace llvm {
namespace wazuhl {

  class Environment {
  public:
    using State = FeatureVector;
    using Action = PassAction;

    Environment(Module &IR, ModuleAnalysisManager &AM);

    bool isInTerminalState();
    void takeAction(const Action &A);
    State getState();
    double getReward();
    PreservedAnalyses getPreservedAnalyses();
  private:
    void updateState();

    Module &IR;
    ModuleAnalysisManager &AM;
    State Current;
    PreservedAnalyses PA;
    bool Terminated = false;
    size_t nTakenActions = 0;
    static constexpr size_t maxAllowedActions = 100;
  };
}
}

#endif /* LLVM_WAZUHL_ENVIRONMENT_H */
