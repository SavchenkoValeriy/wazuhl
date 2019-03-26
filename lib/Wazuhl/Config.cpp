#include "llvm/Wazuhl/Config.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Config/config.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

#include <fstream>

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
FilePath StepCounterFile = appendPath(WazuhlConfigs, "steps.counter");

} // namespace

namespace llvm {
namespace wazuhl {
namespace config {

StringRef getWazuhlConfigPath() { return WazuhlConfigs; }

void ensureConfig() { llvm::sys::fs::create_directories(WazuhlConfigs); }

unsigned getOverallNumberOfSteps() {
  int Result = 0;

  if (!sys::fs::exists(StepCounterFile)) {
    return Result;
  }

  std::ifstream Storage;
  Storage.open(StepCounterFile.data());

  if (!Storage) {
    return Result;
  }

  Storage >> Result;
  Storage.close();

  return Result;
}

void saveOverallNumberOfSteps(unsigned Steps) {
  std::error_code E;
  raw_fd_ostream Storage(StepCounterFile, E, sys::fs::F_RW);

  if (E) {
    return;
  }

  Storage << Steps;
  Storage.close();
}

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
