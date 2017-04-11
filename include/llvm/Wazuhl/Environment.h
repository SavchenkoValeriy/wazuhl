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
  };
}
}

#endif /* LLVM_WAZUHL_ENVIRONMENT_H */
