#include "llvm/Wazuhl/Config.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/Path.h"
#include "llvm/Config/config.h"

namespace {
  template <class PathT, class ... AdditionalT>
  inline PathT appendPath(PathT &Path, AdditionalT ... Appendicies) {
    PathT Result = Path;
    llvm::sys::path::append(Result, Appendicies...);
    return Result;
  }
}

namespace llvm {
namespace wazuhl {
namespace config {

  StringRef getCaffeModelPath() {
    static SmallString<120> RootLLVMDirectory = StringRef{LLVM_PREFIX};
    static SmallString<120> CaffeModelPath =
      appendPath(RootLLVMDirectory, "wazuhl", "model.prototxt");
    return CaffeModelPath;
  }

}
}
}
