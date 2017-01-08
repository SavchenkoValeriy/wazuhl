#include "llvm/Wazuhl/Manager.h"

namespace llvm {
namespace wazuhl {
  PreservedAnalyses Manager::run(Module &IR, AnalysisManager<Module> &AM) {
    PreservedAnalyses PA = PreservedAnalyses::all();

    if (DebugLogging)
      dbgs() << "Starting Wazuhl optimization process.\n";

    ActionList AllActions;

    return PA;
  }
}
}
