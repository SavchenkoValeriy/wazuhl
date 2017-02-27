#ifndef LLVM_WAZUHL_MANAGER_H
#define LLVM_WAZUHL_MANAGER_H

#include "llvm/IR/PassManager.h"

namespace llvm {

namespace wazuhl {

  class Manager : public PassInfoMixin<Manager> {
  public:
    explicit Manager(bool Training = false,
                     bool DebugLogging = false) : Training(Training),
                                                  DebugLogging(DebugLogging) {}

    Manager(Manager &) = delete;
    Manager(Manager &&) = default;

    PreservedAnalyses run(Module &IR, AnalysisManager<Module> &AM);

  private:
    bool Training;
    bool DebugLogging;
  };

}

}

#endif /* LLVM_WAZUHL_MANAGER_H */
