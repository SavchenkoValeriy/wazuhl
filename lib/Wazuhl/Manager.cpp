#include "llvm/Wazuhl/Manager.h"
#include "llvm/Wazuhl/Action.h"
#include <random>

namespace llvm {
namespace wazuhl {
  PreservedAnalyses Manager::run(Module &IR, AnalysisManager<Module> &AM) {
    PreservedAnalyses PA = PreservedAnalyses::all();

    if (DebugLogging)
      dbgs() << "Starting Wazuhl optimization process.\n";

    ActionList AllActions = Action::getAllPossibleActions();

    // this part here is temporal, until actual
    // decision-making mechanism is introduced
    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(0, AllActions.size() - 1);

    for (auto i = 1; i <= 15; ++i) {
      const Action &chosen = AllActions.at(distribution(generator));
      auto *Pass = chosen.takeAction();
      if (!Pass) break; // terminal action has been met

      errs() << "Wazuhl is running " << Pass->name() << "\n";

      PreservedAnalyses PassPA = Pass->run(IR, AM);

      // Update the analysis manager as each pass runs and potentially
      // invalidates analyses.
      AM.invalidate(IR, PassPA);

      // Finally, intersect the preserved analyses to compute the aggregate
      // preserved set for this pass manager.
      PA.intersect(std::move(PassPA));
    }

    return PA;
  }
}
}
