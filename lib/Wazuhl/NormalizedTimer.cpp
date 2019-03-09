#include "llvm/Wazuhl/NormalizedTimer.h"

namespace {
double getCPUTime(llvm::TimeRecord &time) {
  return time.getUserTime() + time.getSystemTime();
}
} // namespace

namespace llvm {
namespace wazuhl {
bool NormalizedTimer::IsInitialized = false;

void NormalizedTimer::init() {
  // this will initiate the creation of a static variable in 'getTimer'
  getTimer();
}

NormalizedTimer::NormalizedTimer()
    : InnerTimer(), Total(), NormalizationTime(0.0) {
  InnerTimer.init(
      "Wazuhl's timer",
      "The purpose of this timer is to add time into Wazuhl's states");
  startInnerTimer();
}

double NormalizedTimer::getTime() { return getTimer().getTimeImpl(); }

double NormalizedTimer::getTimeImpl() {
  stopInnerTimer();
  Total += InnerTimer.getTotalTime();
  startInnerTimer();
  return getCPUTime(Total);
}

double NormalizedTimer::getNormalizedTime() {
  return getTimer().getNormalizedTimeImpl();
}

double NormalizedTimer::getNormalizedTimeImpl() {
  if (!IsInitialized) {
    NormalizationTime = getTimeImpl();
    return 0.0;
  }
  assert(NormalizationTime != 0 && "Normalization time shouldn't be 0!");
  auto Result = getTimeImpl();
  return (Result - NormalizationTime) / NormalizationTime;
}

NormalizedTimer &NormalizedTimer::getTimer() {
  static NormalizedTimer singleton;
  return singleton;
}

void NormalizedTimer::startInnerTimer() { InnerTimer.startTimer(); }

void NormalizedTimer::stopInnerTimer() { InnerTimer.stopTimer(); }

} // namespace wazuhl
} // namespace llvm
