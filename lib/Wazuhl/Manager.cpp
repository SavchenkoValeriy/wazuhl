#include "llvm/Wazuhl/Manager.h"
#include "llvm/Wazuhl/Environment.h"
#include "llvm/Wazuhl/FeatureCollector.h"
#include "llvm/Wazuhl/PolicyEvaluator.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/Support/CommandLine.h"

#include <numeric>

using namespace llvm;
using namespace wazuhl;

cl::opt<bool> TrainingPhase("train-wazuhl",
                            cl::desc("Enable Wazuhl training"),
                            cl::Hidden);

namespace {
  int numberOfNonNullStatistics() {
    auto Statistics = GetStatisticsVector();
    return std::accumulate(Statistics.begin(), Statistics.end(), 0,
                           [](int total, double value) {
                             return total + (value != 0);
                           });
  }

  void registerFeatureCollectors(Module &M, ModuleAnalysisManager &AM) {
    AM.registerPass([] { return ModuleFeatureCollector(); });

    FunctionAnalysisManager &FAM =
      AM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();

    FAM.registerPass([] { return FunctionFeatureCollector(); });
  }

  template <class Evaluator>
  void evaluate(Environment &Env) {
    Evaluator OptimizationEvaluator{Env};
    OptimizationEvaluator.evaluate();
  }

  void train(Environment &Env) {
    evaluate<LearningPolicyEvaluator>(Env);
  }

  void exploit(Environment &Env) {
    evaluate<PolicyEvaluator>(Env);
  }
}

namespace llvm {
namespace wazuhl {
  PreservedAnalyses Manager::run(Module &IR, ModuleAnalysisManager &AM) {
    registerFeatureCollectors(IR, AM);
    Environment OptimizationEnv{IR, AM};

    if (DebugLogging)
      dbgs() << "Starting Wazuhl optimization process.\n";

    if (this->Training || TrainingPhase) {
      train(OptimizationEnv);
    } else {
      exploit(OptimizationEnv);
    }

    return OptimizationEnv.getPreservedAnalyses();
  }
}
}
