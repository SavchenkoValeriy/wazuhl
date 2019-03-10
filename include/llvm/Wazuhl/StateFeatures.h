#ifndef LLVM_WAZUHL_STATE_H
#define LLVM_WAZUHL_STATE_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/Wazuhl/Config.h"
#include "llvm/Wazuhl/PassAction.h"

namespace llvm {
namespace wazuhl {

class StateFeatures {
public:
  using StorageType = SmallVector<double, config::NumberOfFeatures>;
  using ContextType = SmallVector<unsigned, config::ContextSize>;

  StateFeatures() : Storage(config::NumberOfFeatures) {}

  void setIRFeatures(ArrayRef<double> Features) {
    assert(Features.size() == config::NumberOfIRFeatures);
    for (unsigned i = 0; i < config::NumberOfIRFeatures; ++i) {
      Storage[i] = Features[i];
    }
    init();
  }

  void setDiffIRFeatures(ArrayRef<double> Features) {
    assert(Features.size() == config::NumberOfIRFeatures);
    for (unsigned i = 0; i < config::NumberOfIRFeatures; ++i) {
      Storage[config::DiffIRFeaturesOffset + i] = Features[i];
    }
    init();
  }

  void setTime(double time) {
    Storage[config::TimeIndex] = time;
    init();
  }

  void recordAction(const PassAction &A) {
    Context.push_back(A.getIndex());
    init();
  }

  StorageType::pointer data() { return Storage.data(); }
  StorageType::const_pointer data() const { return Storage.data(); }

  StorageType::iterator begin() { return Storage.begin(); }
  StorageType::iterator end() { return Storage.end(); }

  StorageType::const_iterator begin() const { return Storage.begin(); }
  StorageType::const_iterator end() const { return Storage.end(); }

  bool operator==(const StateFeatures &RHS) const {
    return isInitialized() == RHS.isInitialized() and Storage == RHS.Storage;
  }
  bool operator!=(const StateFeatures &RHS) const { return !(*this == RHS); }

  bool operator<(const StateFeatures &RHS) const {
    return Storage < RHS.Storage;
  }

  StorageType::reference operator[](unsigned idx) { return Storage[idx]; }

  StorageType::const_reference operator[](unsigned idx) const {
    return Storage[idx];
  }

  ContextType &getContext() { return Context; }
  const ContextType &getContext() const { return Context; }

  bool isInitialized() const { return Initialized; }

private:
  void init() { Initialized = true; }

  bool Initialized{false};
  StorageType Storage;
  ContextType Context;
};

} // end namespace wazuhl
} // end namespace llvm

#endif /* LLVM_WAZUHL_STATE_H */
