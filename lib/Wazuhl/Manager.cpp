#include "llvm/Wazuhl/Manager.h"
#include "llvm/Wazuhl/Action.h"
#include "llvm/Wazuhl/Config.h"
#include "llvm/Wazuhl/DQN.h"
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
    errs() << "Wazuhl's NN model is stored in " << config::getCaffeModelPath() << "\n";

    DQN Q;

    for (auto i = 1; i <= 15; ++i) {
      // this part here is temporal, until actual
      // decision-making mechanism is introduced
      const Action &A = random::pickOutOf(AllActions);
      OptimizationEnv.takeAction(A);
      if (OptimizationEnv.isInTerminalState()) break; // terminal action has been met

      errs() << "Wazuhl is running " << A.getName() << "\n";
      errs() << "Pass had produced " << numberOfNonNullStatistics() <<
        " non-null statistic values\n";

      auto S = OptimizationEnv.getState();
      Q(S, A) = 15.5;
      llvm::errs() << "Features: \n";
      for (auto value : S) errs() << value << ", ";
      llvm::errs() << "\n\n";
    }

    return OptimizationEnv.getPreservedAnalyses();
  }
}
}
