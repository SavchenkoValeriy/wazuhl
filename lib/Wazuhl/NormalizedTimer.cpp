#include "llvm/Wazuhl/NormalizedTimer.h"

namespace {
  double getCPUTime(llvm::Timer &timer) {
    auto record = timer.getTotalTime();
    return record.getUserTime() + record.getSystemTime();
  }
}

namespace llvm {
namespace wazuhl {
  bool NormalizedTimer::IsInitialized = false;

  NormalizedTimer::Initializer NormalizedTimer::init() {
    if (!IsInitialized) return {};
    return {getTimer()};
  }

  double NormalizedTimer::getTime() {
    return getCPUTime(getTimer().InnerTimer);
  }

  double NormalizedTimer::getNormalizedTime() {
    if (!IsInitialized) return 1.0;
    assert(NormalizationFactor != 0 && "Normalization factor shouldn't be 0!");
    return getTime() / getTimer().NormalizationFactor;
  }

  NormalizedTimer &NormalizedTimer::getTimer() {
    static NormalizedTimer singleton;
    return singleton;
  }

  NormalizedTimer::Initializer::~Initializer() {
    if (Master) {
      Master->NormalizationFactor = NormalizedTimer::getTime();
      NormalizedTimer::IsInitialized = true;
    }
  }
}
}
