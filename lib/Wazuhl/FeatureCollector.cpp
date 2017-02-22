#include "llvm/Wazuhl/FeatureCollector.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"

using namespace llvm;
using namespace wazuhl;

#define DEBUG_TYPE "wazuhl-feature-collector"

namespace {
  class CollectorImpl : public InstVisitor<CollectorImpl> {
    friend class InstVisitor<CollectorImpl>;
  public:
#define HANDLE_INST(N, OPCODE, CLASS)                                   \
    void visit##OPCODE(CLASS &) { ++CollectedFeatures[N]; ++TotalInsts; }
#include "llvm/IR/Instruction.def"

    CollectorImpl() : CollectedFeatures(NumberOfFeatures),
                      TotalInsts(CollectedFeatures[0]) {}

    FeatureVector &getCollectedFeatures() {
      return CollectedFeatures;
    }
  private:
    FeatureVector CollectedFeatures;
    double &TotalInsts;
  };

  void normalizeVector(FeatureVector &features, double normalizationFactor) {
    if (normalizationFactor == 0) return;
    for (auto &feature : features) {
      feature /= normalizationFactor;
    }
  }
}

namespace llvm {
namespace wazuhl {
  FeatureVector FunctionFeatureCollector::run(Function &F, FunctionAnalysisManager &) {
    CollectorImpl Collector;
    Collector.visit(F);
    auto &result = Collector.getCollectedFeatures();
    normalizeVector(result, result[0]);
    return result;
  }

  FeatureVector ModuleFeatureCollector::run(Module &M, ModuleAnalysisManager &AM) {
    using FeatureVectors = std::vector<FeatureVector>;
    FeatureVectors FeaturesOfAllFunctions;

    FunctionAnalysisManager &FAM =
        AM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();

    for (Function &F : M) {
      FeaturesOfAllFunctions.emplace_back(FAM.getResult<FunctionFeatureCollector>(F));
    }

    FeatureVector result(NumberOfFeatures);
    const unsigned NumberOfFunctions = FeaturesOfAllFunctions.size();
    for (unsigned i = 0; i < NumberOfFunctions; ++i) {
      for (unsigned j = 0; j < NumberOfFeatures; ++j) {
        result[j] += FeaturesOfAllFunctions[i][j];
      }
    }

    normalizeVector(result, NumberOfFunctions);
    return result;
  }

  AnalysisKey FunctionFeatureCollector::Key;
  AnalysisKey ModuleFeatureCollector::Key;
}
}
