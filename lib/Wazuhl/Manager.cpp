#include "llvm/Wazuhl/Manager.h"
#include "llvm/Wazuhl/Environment.h"
#include "llvm/Wazuhl/FeatureCollector.h"
#include "llvm/Wazuhl/PolicyEvaluator.h"
#include "llvm/ADT/Statistic.h"

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

    PolicyEvaluator OptimizationEvaluator{OptimizationEnv};
    OptimizationEvaluator.evaluate();

    return OptimizationEnv.getPreservedAnalyses();
  }
}
}
