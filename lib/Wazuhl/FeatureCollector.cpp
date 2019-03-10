#include "llvm/Wazuhl/FeatureCollector.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Module.h"

using namespace llvm;
using namespace wazuhl;
using namespace wazuhl::config;

#define DEBUG_TYPE "wazuhl-feature-collector"

namespace {
class CollectorImpl : public InstVisitor<CollectorImpl> {
  friend class InstVisitor<CollectorImpl>;

public:
#define HANDLE_INST(N, OPCODE, CLASS)                                          \
  void visit##OPCODE(CLASS &) {                                                \
    ++CollectedFeatures[N];                                                    \
    ++TotalInsts;                                                              \
  }
#include "llvm/IR/Instruction.def"

  CollectorImpl()
      : CollectedFeatures(NumberOfFeatures), TotalInsts(CollectedFeatures[0]) {}

  RawFeatureVector &&getCollectedFeatures() {
    return std::move(CollectedFeatures);
  }

private:
  RawFeatureVector CollectedFeatures;
  double &TotalInsts;
};
} // namespace

namespace llvm {
namespace wazuhl {
RawFeatureVector FunctionFeatureCollector::run(Function &F,
                                               FunctionAnalysisManager &) {
  CollectorImpl Collector;
  Collector.visit(F);
  return Collector.getCollectedFeatures();
}

RawIRFeatures ModuleFeatureCollector::run(Module &M,
                                          ModuleAnalysisManager &AM) {
  RawIRFeatures Result;

  FunctionAnalysisManager &FAM =
      AM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();

  for (Function &F : M) {
    if (not F.empty()) {
      Result.addVectorForFunction(&F,
                                  FAM.getResult<FunctionFeatureCollector>(F));
    }
  }

  return Result;
}

AnalysisKey FunctionFeatureCollector::Key;
AnalysisKey ModuleFeatureCollector::Key;
} // namespace wazuhl
} // namespace llvm
