#ifndef LLVM_WAZUHL_FEATURECOLLECTOR_H
#define LLVM_WAZUHL_FEATURECOLLECTOR_H

#include "llvm/IR/PassManager.h"
#include "llvm/ADT/SmallVector.h"

namespace llvm {
namespace wazuhl {
#define LAST_OTHER_INST(num) \
  constexpr int NumberOfOpcodes = num;
#include "llvm/IR/Instruction.def"
  constexpr int NumberOfFeatures = NumberOfOpcodes + 1;
  using FeatureVector = SmallVector<double, NumberOfFeatures>;

  class FunctionFeatureCollector
    : public AnalysisInfoMixin<FunctionFeatureCollector> {
    friend AnalysisInfoMixin<FunctionFeatureCollector>;
    static AnalysisKey Key;

  public:
    using Result = FeatureVector;
    Result run(Function &F, FunctionAnalysisManager &AM);
  };

  class ModuleFeatureCollector
    : public AnalysisInfoMixin<ModuleFeatureCollector> {
    friend AnalysisInfoMixin<ModuleFeatureCollector>;
    static AnalysisKey Key;

  public:
    using Result = FeatureVector;
    Result run(Module &M, ModuleAnalysisManager &AM);
  };
}
}

#endif /* LLVM_WAZUHL_FEATURECOLLECTOR_H */
