#ifndef LLVM_WAZUHL_CONFIG_H
#define LLVM_WAZUHL_CONFIG_H

#include "llvm/ADT/StringRef.h"

namespace llvm {
namespace wazuhl {
namespace config {

  StringRef getCaffeModelPath();
  StringRef getCaffeSolverPath();
  StringRef getWazuhlConfigPath();
  StringRef getTrainedNetFile();
}
}
}

#endif /* LLVM_WAZUHL_CONFIG_H */
