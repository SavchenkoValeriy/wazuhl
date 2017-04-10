#include "llvm/Wazuhl/Environment.h"

using namespace llvm;

namespace {
  template <class PassT>
  inline void runPass(PassT &Pass, Module &IR,
                      ModuleAnalysisManager &AM,
                      PreservedAnalyses &PA) {
      PreservedAnalyses PassPA = Pass->run(IR, AM);

      // Update the analysis manager as each pass runs and potentially
      // invalidates analyses.
      AM.invalidate(IR, PassPA);

      // Finally, intersect the preserved analyses to compute the aggregate
      // preserved set for this pass manager.
      PA.intersect(std::move(PassPA));
  }
}

namespace llvm {
namespace wazuhl {

  Environment::Environment(Module &IR, ModuleAnalysisManager &AM) :
    IR(IR), AM(AM), Current(), PA(PreservedAnalyses::all()) {
    llvm::errs() << "Wazuhl has " << Action::getAllPossibleActions().size() <<
                    " actions to choose from\n";
  }

  Environment::State Environment::getState() {
    return Current;
  }

  void Environment::takeAction(const Action &A) {
    llvm::errs() << "Wazuhl is taking action " << A.getName() << "\n";

    auto Pass = A.takeAction();
    // if pass to run is not there,
    // it is a terminal action and no passes to be ran
    if (!Pass) {
      Terminated = true;
    } else {
      // otherwise we should run the pass that repsents
      // the chosen action
      runPass(Pass, IR, AM, PA);
    }

    // collect features representing the state
    Current = AM.getResult<ModuleFeatureCollector>(IR);
  }

  bool Environment::isInTerminalState() {
    // Terminated is defined not really by the
    // state, but by the last taken action
    return Terminated;
  }

  double Environment::getReward() {
    // TODO: add checking the time threshold
    // and give a fine if the threshold was met

    // no reward till the end of the episode
    return 0.0;
  }

  PreservedAnalyses Environment::getPreservedAnalyses() {
    return PA;
  }

}
}
