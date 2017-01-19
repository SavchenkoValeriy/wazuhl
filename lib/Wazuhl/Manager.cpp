#include "llvm/Wazuhl/Manager.h"
#include "llvm/Wazuhl/Action.h"
#include "llvm/Wazuhl/Environment.h"
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
    Environment OptimizationEnv{IR, AM};
    registerFeatureCollectors(IR, AM);

    if (DebugLogging)
      dbgs() << "Starting Wazuhl optimization process.\n";

    ActionList AllActions = Action::getAllPossibleActions();

    errs() << "Wazuhl has " << AllActions.size() << " actions to choose from\n";

    for (auto i = 1; i <= 15; ++i) {
      // this part here is temporal, until actual
      // decision-making mechanism is introduced
      const Action &chosen = random::pickOutOf(AllActions);
      OptimizationEnv.takeAction(chosen);
      if (OptimizationEnv.isInTerminalState()) break; // terminal action has been met

      errs() << "Wazuhl is running " << chosen.getName() << "\n";
      errs() << "Pass had produced " << numberOfNonNullStatistics() <<
        " non-null statistic values\n";

      auto features = OptimizationEnv.getState();
      llvm::errs() << "Features: \n";
      for (auto value : features) errs() << value << ", ";
      llvm::errs() << "\n\n";
    }

    return OptimizationEnv.getPreservedAnalyses();
  }
}
}
