#include "llvm/Wazuhl/Manager.h"
#include "llvm/Wazuhl/Action.h"
#include "llvm/Wazuhl/FeatureCollector.h"
#include "llvm/Wazuhl/Random.h"
#include "llvm/Wazuhl/ReinforcementLearning.h"
#include "llvm/ADT/Statistic.h"
#include <random>

namespace {
  int numberOfNonNullStatistics() {
    auto Statistics = llvm::GetStatisticsVector();
    return std::accumulate(Statistics.begin(), Statistics.end(), 0,
                           [](int total, double value) {
                             return total + (value != 0);
                           });
  }

  void registerFeatureCollectors(llvm::Module &M, llvm::ModuleAnalysisManager &AM) {
    AM.registerPass([] { return llvm::wazuhl::ModuleFeatureCollector(); });

    llvm::FunctionAnalysisManager &FAM =
      AM.getResult<llvm::FunctionAnalysisManagerModuleProxy>(M).getManager();

    FAM.registerPass([] { return llvm::wazuhl::FunctionFeatureCollector(); });
  }
}

namespace llvm {
namespace wazuhl {
  PreservedAnalyses Manager::run(Module &IR, ModuleAnalysisManager &AM) {
    PreservedAnalyses PA = PreservedAnalyses::all();
    registerFeatureCollectors(IR, AM);

    if (DebugLogging)
      dbgs() << "Starting Wazuhl optimization process.\n";

    ActionList AllActions = Action::getAllPossibleActions();
    EnableStatistics(false /*we don't want to print statistics*/);

    errs() << "Wazuhl has " << AllActions.size() << " actions to choose from\n";

    for (auto i = 1; i <= 15; ++i) {
      // this part here is temporal, until actual
      // decision-making mechanism is introduced
      const Action &chosen = random::pickOutOf(AllActions);
      auto *Pass = chosen.takeAction();
      if (!Pass) break; // terminal action has been met

      errs() << "Wazuhl is running " << chosen.getName() << "\n";

      PreservedAnalyses PassPA = Pass->run(IR, AM);

      errs() << "Pass had produced " << numberOfNonNullStatistics() <<
        " non-null statistic values\n";

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
