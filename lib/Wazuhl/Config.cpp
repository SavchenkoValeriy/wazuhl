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

using FilePath = llvm::SmallString<120>;

FilePath RootLLVMDirectory = llvm::StringRef{LLVM_PREFIX};
FilePath WazuhlConfigs = appendPath(RootLLVMDirectory, "wazuhl");

} // namespace

namespace llvm {
namespace wazuhl {
namespace config {

StringRef getWazuhlConfigPath() { return WazuhlConfigs; }

void ensureConfig() { llvm::sys::fs::create_directories(WazuhlConfigs); }


StringRef getTrainingNetFile() {
  static FilePath TrainingNet = appendPath(WazuhlConfigs, "training.model");
  return TrainingNet;
}

StringRef getTargetNetFile() {
  static FilePath TargetNet = appendPath(WazuhlConfigs, "target.model");
  return TargetNet;
}
} // namespace config
} // namespace wazuhl
} // namespace llvm
