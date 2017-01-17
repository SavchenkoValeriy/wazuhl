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

    FeatureVector getCollectedFeatures() {
      return CollectedFeatures;
    }
  private:
    FeatureVector CollectedFeatures;
    double &TotalInsts;
  };
}

namespace llvm {
namespace wazuhl {
  FeatureVector FunctionFeatureCollector::run(Function &F, FunctionAnalysisManager &) {
    CollectorImpl Collector;
    Collector.visit(F);
    return Collector.getCollectedFeatures();
  }

  FeatureVector ModuleFeatureCollector::run(Module &M, ModuleAnalysisManager &AM) {
    using FeatureVectors = std::vector<FeatureVector>;
    FeatureVectors FeaturesOfAllFunctions;

    FunctionAnalysisManager &FAM =
        AM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();

    for (Function &F : M) {
      FeaturesOfAllFunctions.emplace_back(FAM.getResult<FunctionFeatureCollector>(F));
    }

    FeatureVector result(NumberOfOpcodes);
    for (unsigned i = 0; i < FeaturesOfAllFunctions.size(); ++i) {
      for (unsigned j = 0; j < NumberOfFeatures; ++j) {
        result[j] += FeaturesOfAllFunctions[i][j];
      }
    }
    return result;
  }

  AnalysisKey FunctionFeatureCollector::Key;
  AnalysisKey ModuleFeatureCollector::Key;
}
}
