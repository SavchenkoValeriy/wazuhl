#include "llvm/Wazuhl/Config.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Config/config.h"
#include "llvm/Support/Path.h"

namespace {
template <class PathT, class... AdditionalT>
inline PathT appendPath(PathT &Path, AdditionalT... Appendicies) {
  PathT Result = Path;
  llvm::sys::path::append(Result, Appendicies...);
  return Result;
}

using SmallString = llvm::SmallString<120>;

SmallString RootLLVMDirectory = llvm::StringRef{LLVM_PREFIX};
SmallString WazuhlConfigs = appendPath(RootLLVMDirectory, "wazuhl");
} // namespace

namespace llvm {
namespace wazuhl {
namespace config {

StringRef getWazuhlConfigPath() { return WazuhlConfigs; }

StringRef getTrainedNetFile() {
  static SmallString<120> TrainedNet =
      appendPath(WazuhlConfigs, "trained.model");
  return TrainedNet;
}
} // namespace config
} // namespace wazuhl
} // namespace llvm
