#ifndef LLVM_WAZUHL_MANAGER_H
#define LLVM_WAZUHL_MANAGER_H

#include <llvm/IR/PassManager.h>

namespace llvm {

namespace wazuhl {

  class Manager : public PassInfoMixin<Manager> {
  public:
    explicit Manager(bool DebugLogging = false) : DebugLogging(DebugLogging) {}

    Manager(Manager &) = delete;
    Manager(Manager &&) = default;

    PreservedAnalyses run(Module &IR, AnalysisManager<Module> &AM) {
      PreservedAnalyses PA = PreservedAnalyses::all();

      if (DebugLogging)
        dbgs() << "Starting Wazuhl optimization process.\n";

      // do nothing for now

      return PA;
    }

  private:
    bool DebugLogging;
  };

}

}

#endif /* LLVM_WAZUHL_MANAGER_H */
